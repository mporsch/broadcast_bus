#ifndef MESSAGE_BUS_IMPL_H
#define MESSAGE_BUS_IMPL_H

#ifndef MESSAGE_BUS_H
 #error "include via message_bus.h only"
#endif // MESSAGE_BUS_H

#include <algorithm>
#include <cassert>
#include <vector>

template<typename Message>
class MessageBusTerminal;

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

    Terminals terminals;
    std::mutex mtx;

    MessageBusImpl()
    {}

    ~MessageBusImpl()
    {
      // all terminals must be closed before destroying the bus
      assert(terminals.empty());
    }

    template<typename... Args>
    std::future<void> tx(Terminal *parent, Args... args)
    {
      return std::future<void>();
    }

    MessagePtr rx_nonblocking(Terminal *parent)
    {
      return std::make_shared<Message>();
    }

    std::future<MessagePtr> rx_blockable(Terminal *parent)
    {
      std::promise<MessagePtr> promise;
      promise.set_value(std::make_shared<Message>());
      return promise.get_future();
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
typename MessageBus<Message>::Terminal MessageBus<Message>::GetTerminal()
{
  return Terminal(new MessageBusTerminal<Message>(m_impl));
}

#endif // MESSAGE_BUS_IMPL_H
