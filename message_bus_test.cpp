#include "message_bus.h"

#include <iostream>
#include <thread>

void ThreadOne(MessageBus::Terminal terminal)
{

}

void ThreadTwo(MessageBus::Terminal terminal)
{

}

void ThreadThree(MessageBus::Terminal terminal)
{

}

int main(int argc, char **argv)
{
  MessageBus messageBus;

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
