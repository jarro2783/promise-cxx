#ifndef PROMISE_NET_NET_HPP_INCLUDED
#define PROMISE_NET_NET_HPP_INCLUDED

#include <arpa/inet.h>

#include <list>
#include <unordered_map>

#include <promise/ev.hpp>
#include <promise.hpp>
#include <ev++.h>

namespace promise
{
  namespace net
  {
    class NetIO
    {
      public:

      NetIO(NetIO&&) = delete;

      NetIO(std::function<void(ev::io&, int)> func)
      : m_func(func)
      {
      }

      template <typename... Args>
      void
      operator()(Args&&... args) const
      {
        m_func(std::forward<Args>(args)...);
      }

      private:

      ev::io m_io;
      std::function<void(ev::io&, int)> m_func;
    };

    typedef std::function<void(int, sockaddr_in* client)> ListenCallback;

    class Listener
    {
      public:

      Listener(const std::string& address, ListenCallback accepted);

      void
      operator()(ev::io&, int);

      private:

      ListenCallback m_accepted;
      ev::io m_listen_io;
    };

    class Loop
    {
      public:
      Loop()
      {
        m_sig.set(this);
        m_sig.set(SIGINT);
        m_sig.start();
      }

      void
      run()
      {
        m_loop.run();
      }

      void
      operator()(ev::sig&, int)
      {
        m_loop.unloop();
      }

      private:
      ev::sig m_sig;
      ev::default_loop m_loop;
    };

    class NetConnection
    {
      public:
      ::Promise<int, int>
      connect(const std::string& address, int port);

      void
      operator()(ev::io& watcher, int events);

      private:
      ev::io m_io_watcher;
      std::list<NetIO> m_handleStore;
      std::unordered_map<int, decltype(m_handleStore)::const_iterator> m_handlers;
    };

    class Connections
    {
      public:

      Connections() = default;
      Connections(const Connections&) = delete;

      ::Promise<void, int>
      read(int socket);

      // readline should go here
      //
      // so should write
      //
      // what would a generic HTTP protocol with promises look like?

      ::Promise<std::string, int>
      readline(int socket);

      void
      operator()(ev::io&, int);

      void
      shutdown()
      {
        m_reader.clear();
      }

      private:

      typedef std::tuple<
        std::shared_ptr<ev::io>,
        std::function<void()>,
        std::function<void(int)>
      > SocketHandler;

      typedef std::tuple<
        std::shared_ptr<ev::io>,
        std::function<void(ev::io&)>
      > ReadHandler;

      std::unordered_map<int, ReadHandler> m_reader;
    };
  }
}

#endif
