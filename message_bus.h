#include <memory>

namespace message_bus_detail
{
  struct MessageBusImpl
  {
    MessageBusImpl()
    {}

    ~MessageBusImpl()
    {}
  };
}

class MessageBus;

class MessageBusTerminal
{
  friend class MessageBus;

private:
  MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl> impl)
    : m_impl(std::move(impl))
  {}

public:
  ~MessageBusTerminal()
  {}

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl> m_impl;
};

class MessageBus
{
public:
  using Terminal = std::unique_ptr<MessageBusTerminal>;

public:
  MessageBus()
    : m_impl(std::make_shared<message_bus_detail::MessageBusImpl>())
  {}

  ~MessageBus()
  {}

  Terminal GetTerminal()
  {
    return Terminal(new MessageBusTerminal(m_impl));
  }

private:
  std::shared_ptr<message_bus_detail::MessageBusImpl> m_impl;
};
