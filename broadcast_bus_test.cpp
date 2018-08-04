#include "broadcast_bus.h"

#include <cassert>
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

    auto message = terminal->rx_nonblocking();
    if(message) {
      switch(*message) {
        case READY:
          break;
        case STEADY:
        {
          // don't hold on to the received message while waiting
          message.reset();

          std::unique_lock<std::mutex> lock(printMutex);
          auto futureTx = terminal->tx(GO);
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

    switch(*terminal->rx_blockable().get()) {
      case READY:
      {
        std::unique_lock<std::mutex> lock(printMutex);
        auto futureTx = terminal->tx(STEADY);
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

void ThreadThree(Bus::Terminal terminal)
{
  DoStuff();

  {
    std::unique_lock<std::mutex> lock(printMutex);
    auto futureTx = terminal->tx(READY);
    std::cout << "sending READY..." << std::flush;
    futureTx.wait();
    std::cout << " done" << std::endl;
  }

  for(;;) {
    DoStuff();

    switch(*terminal->rx_blockable().get()) {
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

int main(int /*argc*/, char **/*argv*/)
{
  Bus broadcastBus;

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

  return EXIT_SUCCESS;
}
