#include "message_bus.h"

#include <iostream>
#include <thread>

using Bus = MessageBus<char>;

void ThreadOne(Bus::Terminal terminal)
{
  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto message = terminal->rx_nonblocking();
  std::cout << "1: received " << (message ? "a" : "no") << " message\n";
}

void ThreadTwo(Bus::Terminal terminal)
{
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto futureMessage = terminal->rx_blocking();
  std::cout << "2: received " << (futureMessage.get() ? "a" : "no") << " message\n";
}

void ThreadThree(Bus::Terminal terminal)
{
  std::this_thread::sleep_for(std::chrono::seconds(3));
  terminal->tx(1);
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
