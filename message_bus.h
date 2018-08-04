#include <algorithm>
#include <cassert>
#include <future>
#include <memory>
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
}

template<typename Message>
class MessageBus;

template<typename Message>
class MessageBusTerminal
{
  friend class MessageBus<Message>;

public:
  using MessagePtr = std::shared_ptr<Message>;

private:
  MessageBusTerminal(std::shared_ptr<message_bus_detail::MessageBusImpl<Message>> impl)
    : m_impl(std::move(impl))
  {
    m_impl->CreateTerminal(this);
  }

public:
  ~MessageBusTerminal()
  {
    m_impl->CloseTerminal(this);
  }

  template<typename... Args>
  std::future<void> tx(Args... args)
  {
    return m_impl->tx(this, std::forward<Args>(args)...);
  }

  MessagePtr rx_nonblocking()
  {
    return m_impl->rx_nonblocking(this);
  }

  std::future<MessagePtr> rx_blockable()
  {
    return m_impl->rx_blockable(this);
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
