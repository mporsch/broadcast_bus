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
      Terminal const *parent; // parent address used as ID
      std::queue<MessagePtr> rxq; // queue of incoming Message
      std::unique_ptr<std::promise<void>> promise; // RX promise to fulfill

      TerminalImpl(Terminal const *parent)
        : parent(parent)
      {}

      bool operator==(TerminalImpl const &other) const
      {
        return (parent == other.parent);
      }
    };

    struct Terminals : public std::vector<TerminalImpl>
    {
      typename std::vector<TerminalImpl>::const_iterator find(Terminal const *parent) const
      {
        return std::find(this->begin(), this->end(), parent);
      }

      typename std::vector<TerminalImpl>::iterator find(Terminal const *parent)
      {
        return std::find(this->begin(), this->end(), parent);
      }
    };

    struct MessageAck
    {
      std::promise<void> promise; // promise to fulfill for TX

      void operator()(Message *message)
      {
        if(message) {
          promise.set_value();
          delete message;
        }
      }
    };

    Terminals terminals; // list of attached terminals
    mutable std::mutex mtx; // concurrent access mutex

    ~BroadcastBusImpl()
    {
      // all terminals must be closed before destroying the bus
      assert(terminals.empty());
    }

    template<typename... Args>
    std::future<void> Tx(Terminal const *parent, Args... args)
    {
      std::lock_guard<std::mutex> lock(mtx);

      if(terminals.size() > 1U) {
        // dumb pointer to avoid unneccesary shared_ptr copies
        MessagePtr *message = nullptr;

        // share the message with all terminals except the transmitter
        for(auto &&terminal : terminals) {
          if(terminal.parent == parent) {
          } else {
            if(message) {
              terminal.rxq.emplace(*message);
            } else { // create the message with a custom deleter as receipt acknowledgement
              terminal.rxq.emplace(new Message(std::forward<Args>(args)...), MessageAck{});
              message = &terminal.rxq.back();
            }

            // notify if the terminal was waiting for RX
            if (terminal.promise) {
              terminal.promise->set_value();
              terminal.promise.reset();
            }
          }
        }
        assert(message);

        // return the future for receipt acknowledgement
        return std::get_deleter<MessageAck>(*message)->promise.get_future();
      } else {
        std::promise<void> promise;
        promise.set_value();
        return promise.get_future();
      }
    }

    bool IsRxReady(Terminal const *parent) const
    {
      std::lock_guard<std::mutex> lock(mtx);

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      return !terminal->rxq.empty();
    }

    std::future<void> RxReady(Terminal const *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      if(terminal->rxq.empty()) {
        // keep a promise for the next received message
        terminal->promise = std::make_unique<std::promise<void>>();
        return terminal->promise->get_future();
      } else {
        // create a promise to fulfill immediately
        std::promise<void> promise;
        promise.set_value();
        return promise.get_future();
      }
    }

    MessagePtr Rx(Terminal const *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      MessagePtr ret{};

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      if(terminal->rxq.empty()) {
        // if there are no messages queued return a nullptr
      } else {
        // return the oldest queued message
        ret = std::move(terminal->rxq.front());
        terminal->rxq.pop();
      }

      return ret;
    }

    void CreateTerminal(Terminal const *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      terminals.emplace_back(parent);
    }

    void CloseTerminal(Terminal const *parent)
    {
      std::lock_guard<std::mutex> lock(mtx);

      auto const terminal = terminals.find(parent);
      assert(terminal != std::end(terminals));
      (void)terminals.erase(terminal);
    }
  };
} // namespace broadcast_bus_detail

template<typename Message>
BroadcastBusTerminal<Message>::BroadcastBusTerminal(
  std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> impl)
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
std::future<void> BroadcastBusTerminal<Message>::Tx(Args... args)
{
  return m_impl->Tx(this, std::forward<Args>(args)...);
}

template<typename Message>
bool BroadcastBusTerminal<Message>::IsRxReady() const
{
  return m_impl->IsRxReady(this);
}

template<typename Message>
std::future<void> BroadcastBusTerminal<Message>::RxReady()
{
  return m_impl->RxReady(this);
}

template<typename Message>
typename BroadcastBusTerminal<Message>::MessagePtr
BroadcastBusTerminal<Message>::Rx()
{
  return m_impl->Rx(this);
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
