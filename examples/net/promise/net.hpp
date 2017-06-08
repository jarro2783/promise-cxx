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

      void
      operator()(ev::io&, int);

      void
      shutdown()
      {
        m_handlers.clear();
      }

      private:
      typedef std::tuple<
        std::shared_ptr<ev::io>,
        std::function<void()>,
        std::function<void(int)>
      > SocketHandler;
      std::unordered_map<int, SocketHandler> m_handlers;
    };

    class FileIO
    {
      public:
      FileIO(int socket, int mode)
      : m_socket(socket)
      {
        m_io.set(socket, mode);
        m_io.set(this);
        m_io.start();
      }

      ~FileIO()
      {
        std::cout << "Destroyed FileIO" << std::endl;
      }

      void
      operator()(ev::io& io, int events)
      {
        // this is so dirty
        // I need to manage this somehow
        std::unique_ptr<FileIO> ptr(this);

        if (events & ev::READ)
        {
          m_reader(m_socket);
        }

        if (events & ev::WRITE)
        {
          m_writer(m_socket);
        }
      }

      void
      set_reader(std::function<void(int)> func)
      {
        m_reader = func;
      }

      private:

      int m_socket;

      std::function<void(int)> m_reader;
      std::function<void(int)> m_writer;

      ev::io m_io;
    };

    ::Promise<std::string, int>
    readline(int socket);

    void run();
  }
}

#endif
