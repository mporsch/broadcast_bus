#ifndef BROADCAST_BUS_H
#define BROADCAST_BUS_H

#include <future>
#include <memory>

namespace broadcast_bus_detail
{
  template<typename Message>
  struct BroadcastBusImpl;
} // namespace broadcast_bus_detail

template<typename Message>
class BroadcastBus;

template<typename Message>
class BroadcastBusTerminal
{
  friend class BroadcastBus<Message>;

public:
  using MessagePtr = std::shared_ptr<Message>;

private:
  /// created by BroadcastBus only
  BroadcastBusTerminal(std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> impl);

public:
  ~BroadcastBusTerminal();

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
  std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> m_impl;
};

template<typename Message>
class BroadcastBus
{
public:
  using Terminal = std::unique_ptr<BroadcastBusTerminal<Message>>;

public:
  BroadcastBus();
  ~BroadcastBus();

  /// attach an RX/TX terminal to the bus
  /// will only receive future messages
  Terminal AttachTerminal();

private:
  std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> m_impl;
};

#include "broadcast_bus_impl.h"

#endif // BROADCAST_BUS_H
