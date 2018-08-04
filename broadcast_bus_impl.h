#ifndef BROADCAST_BUS_IMPL_H
#define BROADCAST_BUS_IMPL_H

#ifndef BROADCAST_BUS_H
 #error "include via broadcast_bus.h only"
#endif // BROADCAST_BUS_H

#include <algorithm>
#include <cassert>
#include <queue>
#include <vector>

namespace broadcast_bus_detail
{
  template<typename Message>
  struct BroadcastBusImpl
  {
    using Terminal = BroadcastBusTerminal<Message>;
    using MessagePtr = typename Terminal::MessagePtr;

    struct TerminalImpl
    {
      Terminal *parent; // parent address used as ID
      std::queue<MessagePtr> rxQueue; // queue of incoming Message
      std::unique_ptr<std::promise<MessagePtr>> rxPromise; // promise to fulfill for rx_blockable()

      TerminalImpl(Terminal *parent)
        : parent(parent)
      {}
    };

    struct Terminals : public std::vector<TerminalImpl>
    {
      typename std::vector<TerminalImpl>::iterator find(Terminal *parent)
      {
        return std::find_if(this->begin(), this->end(),
          [&](TerminalImpl const &terminal) -> bool
          {
            return (parent == terminal.parent);
          });
      }
    };

    struct MessageAck
    {
      std::promise<void> promise; // promise to fulfill for tx()

      void operator()(Message *message)
      {
        if(message) {
          promise.set_value();
          delete message;
        }
      }
    };

    Terminals terminals; // list of attached terminals
    std::mutex mtx; // concurrent access mutex

    ~BroadcastBusImpl()
    {
      // all terminals must be closed before destroying the bus
      assert(terminals.empty());
    }

    template<typename... Args>
    std::future<void> tx(Terminal *parent, Args... args)
    {
      std::lock_guard<std::mutex> lock(mtx);

      // create the message with a custom deleter as receipt acknowledgement
      auto message = MessagePtr(new Message(std::forward<Args>(args)...), MessageAck{});

      // forward the message to all terminals except the transmitter
      for(auto it = std::begin(terminals); it != std::end(terminals); ++it) {
        if(it->parent == parent) {
        } else if (it->rxPromise) {
          it->rxPromise->set_value(message);
          it->rxPromise.reset();
        } else {
          it->rxQueue.push(message); // TODO final move instead of copy
        }
      }

      // return the future for receipt acknowledgement
      return std::get_deleter<MessageAck>(message)->promise.get_future();
    }

    MessagePtr rx_nonblocking(Terminal *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      MessagePtr ret{};

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      if(terminal->rxQueue.empty()) {
        // if there are no messages queued return a nullptr
      } else {
        // return the oldest queued message
        ret = std::move(terminal->rxQueue.front());
        terminal->rxQueue.pop();
      }

      return ret;
    }

    std::future<MessagePtr> rx_blockable(Terminal *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      if(terminal->rxQueue.empty()) {
        // keep a promise for the next received message
        terminal->rxPromise = std::make_unique<std::promise<MessagePtr>>();
        return terminal->rxPromise->get_future();
      } else {
        // create a promise to fulfill immediately with the oldest queued message
        std::promise<MessagePtr> promise;
        promise.set_value(std::move(terminal->rxQueue.front()));
        terminal->rxQueue.pop();
        return promise.get_future();
      }
    }

    void CreateTerminal(Terminal *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      terminals.emplace_back(parent);
    }

    void CloseTerminal(Terminal *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      auto const it = terminals.find(parent);
      assert(it != std::end(terminals));
      (void)terminals.erase(it);
    }
  };
} // namespace broadcast_bus_detail

template<typename Message>
BroadcastBusTerminal<Message>::BroadcastBusTerminal(std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> impl)
  : m_impl(std::move(impl))
{
  m_impl->CreateTerminal(this);
}

template<typename Message>
BroadcastBusTerminal<Message>::~BroadcastBusTerminal()
{
  m_impl->CloseTerminal(this);
}

template<typename Message>
template<typename... Args>
std::future<void> BroadcastBusTerminal<Message>::tx(Args... args)
{
  return m_impl->tx(this, std::forward<Args>(args)...);
}

template<typename Message>
typename BroadcastBusTerminal<Message>::MessagePtr
BroadcastBusTerminal<Message>::rx_nonblocking()
{
  return m_impl->rx_nonblocking(this);
}

template<typename Message>
std::future<typename BroadcastBusTerminal<Message>::MessagePtr>
BroadcastBusTerminal<Message>::rx_blockable()
{
  return m_impl->rx_blockable(this);
}


template<typename Message>
BroadcastBus<Message>::BroadcastBus()
  : m_impl(std::make_shared<broadcast_bus_detail::BroadcastBusImpl<Message>>())
{}

template<typename Message>
BroadcastBus<Message>::~BroadcastBus()
{}

template<typename Message>
typename BroadcastBus<Message>::Terminal BroadcastBus<Message>::AttachTerminal()
{
  return Terminal(new BroadcastBusTerminal<Message>(m_impl));
}

#endif // BROADCAST_BUS_IMPL_H
