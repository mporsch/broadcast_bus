#include <future>
#include <memory>

namespace message_bus_detail
{
  template<typename Message>
  struct MessageBusImpl
  {
    MessageBusImpl()
    {}

    ~MessageBusImpl()
    {}
  };
}

template<typename Message>
class MessageBus;

template<typename Message>
class MessageBusTerminal
{
  friend class MessageBus<Message>;

private:
  MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> impl)
    : m_impl(std::move(impl))
  {}

public:
  ~MessageBusTerminal()
  {}

  template<typename... Args>
  std::future<void> tx(Args... args)
  {
    return std::future<void>();
  }

  std::shared_ptr<Message> rx_nonblocking()
  {
    return std::make_shared<Message>();
  }

  std::future<std::shared_ptr<Message>> rx_blocking()
  {
    std::promise<std::shared_ptr<Message>> promise;
    promise.set_value(std::make_shared<Message>());
    return promise.get_future();
  }

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> m_impl;
};

template<typename Message>
class MessageBus
{
public:
  using Terminal = std::unique_ptr<MessageBusTerminal<Message>>;

public:
  MessageBus()
    : m_impl(std::make_shared<message_bus_detail::MessageBusImpl<Message>>())
  {}

  ~MessageBus()
  {}

  Terminal GetTerminal()
  {
    return Terminal(new MessageBusTerminal<Message>(m_impl));
  }

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> m_impl;
};
