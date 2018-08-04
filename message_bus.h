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
  /// created by MessageBus only
  MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> impl);

public:
  ~MessageBusTerminal();

  /// transmit a message to all attached terminals
  /// the returned future indicates complete receipt
  template<typename... Args>
  std::future<void> tx(Args... args);

  /// check for a received message
  /// the returned pointer may be null
  MessagePtr rx_nonblocking();

  /// get a future for a message to be received
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

  /// attach an RX/TX terminal to the bus
  /// will only receive future messages
  Terminal AttachTerminal();

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> m_impl;
};

#include "message_bus_impl.h"

#endif // MESSAGE_BUS_H
