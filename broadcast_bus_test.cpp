#include "broadcast_bus.h"

#include <cassert>
#include <csignal>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

enum Message
{
  READY
, STEADY
, GO
};
using Bus = BroadcastBus<Message>;

std::mutex printMutex;  ///< mutex to unmangle cout output
bool shouldQuit = false;  ///< shutdown flag

void DoStuff()
{
  static std::default_random_engine generator;
  static std::uniform_int_distribution<int> distribution(1, 5000);

  // simulate some processing delay
  std::this_thread::sleep_for(std::chrono::milliseconds(distribution(generator)));
}

void ThreadOne(Bus::Terminal terminal)
{
  for(;;) {
    DoStuff();

    auto message = terminal->Rx();
    if(message) {
      switch(*message) {
        case READY:
          break;
        case STEADY:
        {
          // don't hold on to the received message while waiting
          message.reset();

          std::unique_lock<std::mutex> lock(printMutex);
          auto futureTx = terminal->Tx(GO);
          std::cout << "sending GO..." << std::flush;
          futureTx.wait();
          std::cout << " done" << std::endl;
          return;
        }
        case GO:
        default:
          assert(false);
      }
    }
  }
}

void ThreadTwo(Bus::Terminal terminal)
{
  for(;;) {
    DoStuff();

    if(terminal->IsRxReady()) {
      auto message = terminal->Rx();
      assert(message);
      switch(*message) {
        case READY:
        {
          // don't hold on to the received message while waiting
          message.reset();

          std::unique_lock<std::mutex> lock(printMutex);
          auto futureTx = terminal->Tx(STEADY);
          std::cout << "sending STEADY..." << std::flush;
          futureTx.wait();
          std::cout << " done" << std::endl;
          break;
        }
        case STEADY:
          assert(false);
        case GO:
        default:
          return;
      }
    }
  }
}

void ThreadThree(Bus::Terminal terminal)
{
  DoStuff();

  {
    std::unique_lock<std::mutex> lock(printMutex);
    auto futureTx = terminal->Tx(READY);
    std::cout << "sending READY..." << std::flush;
    futureTx.wait();
    std::cout << " done" << std::endl;
  }

  for(;;) {
    DoStuff();

    terminal->RxReady().wait();
    switch(*terminal->Rx()) {
      case READY:
        assert(false);
      case STEADY:
        break;
      case GO:
      default:
        return;
    }
  }
}

void ThreadFour(Bus::Terminal terminal)
{
  DoStuff();

  (void)terminal;
  // leave the bus without receiving anything
}

void Shutdown(int)
{
  shouldQuit = true;
}

int main(int /*argc*/, char **/*argv*/)
{
  if(signal(SIGINT, Shutdown))
    return EXIT_FAILURE;

  Bus broadcastBus;

  while(!shouldQuit) {
    std::thread threads[] {
      std::thread(ThreadOne, broadcastBus.AttachTerminal())
    , std::thread(ThreadTwo, broadcastBus.AttachTerminal())
    , std::thread(ThreadThree, broadcastBus.AttachTerminal())
    , std::thread(ThreadFour, broadcastBus.AttachTerminal())
    };

    // after the terminals are attached the BroadcastBus may as well go out of scope

    for(auto &&t : threads)
      if(t.joinable())
        t.join();
  }

  return EXIT_SUCCESS;
}
