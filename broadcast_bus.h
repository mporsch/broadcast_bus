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

  /// transmit a message to all other attached terminals
  /// @return  future that indicates receipt at all other terminals
  template<typename... Args>
  std::future<void> Tx(Args... args);

  /// check if a received message is available
  /// @note  may return false negatives under race conditions,
  ///        but not false positives
  bool IsRxReady() const;

  /// get a future for a message to be received
  /// @note  timeout-waiting on the future may cause false negatives
  ///        under race conditions, but not false positives
  std::future<void> RxReady();

  /// check for a received message
  /// @return  message pointer that may be null if IsRxReady()
  ///          or WaitRxReady() have not signalled success
  /// @note  releasing the message triggers the TX acknowledgement
  /// @note  release the message before sleeping to avoid bus deadlocks
  MessagePtr Rx();

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
  /// @note  will only receive future messages
  Terminal AttachTerminal();

private:
  std::shared_ptr<broadcast_bus_detail::BroadcastBusImpl<Message>> m_impl;
};

#include "broadcast_bus_impl.h"

#endif // BROADCAST_BUS_H
