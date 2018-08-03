#include "message_bus.h"

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
using Bus = MessageBus<Message>;

std::mutex printMutex;                                     ///< Mutex to unmangle cout output
std::default_random_engine generator;                      ///< Random generator
std::uniform_int_distribution<int> distribution(1, 5000);  ///< Random distribution

void DoStuff()
{
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
          std::unique_lock<std::mutex> lock(printMutex);
          auto futureTx = terminal->tx(GO);
          std::cout << "sending GO...";
          futureTx.wait();
          std::cout << " done\n";
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

    auto futureMessage = terminal->rx_blockable();
    auto message = futureMessage.get();
    switch(*message) {
      case READY:
      {
        std::unique_lock<std::mutex> lock(printMutex);
        auto futureTx = terminal->tx(STEADY);
        std::cout << "sending STEADY...";
        futureTx.wait();
        std::cout << " done\n";
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
    std::cout << "sending READY...";
    futureTx.wait();
    std::cout << " done\n";
  }

  for(;;) {
    DoStuff();

    auto message = terminal->rx_blockable().get();
    switch(*message) {
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

int main(int argc, char **argv)
{
  Bus messageBus;

  std::thread threads[] {
    std::thread(ThreadOne, messageBus.GetTerminal())
  , std::thread(ThreadTwo, messageBus.GetTerminal())
  , std::thread(ThreadThree, messageBus.GetTerminal())
  };

  for(auto &&t : threads)
    if(t.joinable())
      t.join();

  return EXIT_SUCCESS;
}
