#ifndef MESSAGE_BUS_IMPL_H
#define MESSAGE_BUS_IMPL_H

#ifndef MESSAGE_BUS_H
 #error "include via message_bus.h only"
#endif // MESSAGE_BUS_H

#include <algorithm>
#include <cassert>
#include <queue>
#include <vector>

namespace message_bus_detail
{
  template<typename Message>
  struct MessageBusImpl
  {
    using Terminal = MessageBusTerminal<Message>;
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

    ~MessageBusImpl()
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
      auto transmitter = terminals.find(parent);
      assert(transmitter != std::end(terminals));
      for(auto it = std::begin(terminals); it != std::end(terminals); ++it) {
        if(it == transmitter) {
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
} // namespace message_bus_detail

template<typename Message>
MessageBusTerminal<Message>::MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> impl)
  : m_impl(std::move(impl))
{
  m_impl->CreateTerminal(this);
}

template<typename Message>
MessageBusTerminal<Message>::~MessageBusTerminal()
{
  m_impl->CloseTerminal(this);
}

template<typename Message>
template<typename... Args>
std::future<void> MessageBusTerminal<Message>::tx(Args... args)
{
  return m_impl->tx(this, std::forward<Args>(args)...);
}

template<typename Message>
typename MessageBusTerminal<Message>::MessagePtr
MessageBusTerminal<Message>::rx_nonblocking()
{
  return m_impl->rx_nonblocking(this);
}

template<typename Message>
std::future<typename MessageBusTerminal<Message>::MessagePtr>
MessageBusTerminal<Message>::rx_blockable()
{
  return m_impl->rx_blockable(this);
}


template<typename Message>
MessageBus<Message>::MessageBus()
  : m_impl(std::make_shared<message_bus_detail::MessageBusImpl<Message>>())
{}

template<typename Message>
MessageBus<Message>::~MessageBus()
{}

template<typename Message>
typename MessageBus<Message>::Terminal MessageBus<Message>::AttachTerminal()
{
  return Terminal(new MessageBusTerminal<Message>(m_impl));
}

#endif // MESSAGE_BUS_IMPL_H
