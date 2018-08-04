#ifndef MESSAGE_BUS_H
#define MESSAGE_BUS_H

#include <future>
#include <memory>

namespace message_bus_detail
{
  template<typename Message>
  struct MessageBusImpl;
} // namespace message_bus_detail

template<typename Message>
class MessageBus;

template<typename Message>
class MessageBusTerminal
{
  friend class MessageBus<Message>;

public:
  using MessagePtr = std::shared_ptr<Message>;

private:
  MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> impl);

public:
  ~MessageBusTerminal();

  template<typename... Args>
  std::future<void> tx(Args... args);

  MessagePtr rx_nonblocking();

  std::future<MessagePtr> rx_blockable();

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> m_impl;
};

template<typename Message>
class MessageBus
{
public:
  using Terminal = std::unique_ptr<MessageBusTerminal<Message>>;

public:
  MessageBus();
  ~MessageBus();

  Terminal GetTerminal();

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> m_impl;
};

#include "message_bus_impl.h"

#endif // MESSAGE_BUS_H
