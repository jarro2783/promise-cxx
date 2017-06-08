#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <promise/net.hpp>

namespace promise
{
namespace net
{

void
NetConnection::operator()(ev::io& io, int events)
{
  auto iter = m_handlers.find(io.fd);

  if (iter == m_handlers.end())
  {
    throw "Unable to find socket handler";
  }

  (*iter->second)(io, events);

  io.stop();
  m_handleStore.erase(iter->second);
  m_handlers.erase(iter);
}

::Promise<int, int>
NetConnection::connect(const std::string& address, int port)
{
  return promise::Promise<int, int>::create(
    [&] (auto resolve, auto reject)
    {
      int result = -1;
      int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
      if (fd == -1)
      {
        reject(errno);
        return;
      }

      struct sockaddr_in addr{
        AF_INET,
        htons(port),
        inet_addr(address.c_str())
      };

      result = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

      if (result == -1 && errno != EINPROGRESS)
      {
        close(fd);
        reject(errno);
        return;
      }

      auto handler =
        [resolve, reject] (ev::io& io, int events) {
          int value = 0;
          socklen_t len = sizeof(value);
          getsockopt(io.fd, SOL_SOCKET, SO_ERROR, &value, &len);

          if (value != 0)
          {
            reject(value);
          }
          else
          {
            resolve(io.fd);
          }
        };

      m_handleStore.emplace_front(handler);
      m_handlers.emplace(fd, m_handleStore.begin());

      m_io_watcher.set(fd, ev::WRITE);
      m_io_watcher.set(this);
      m_io_watcher.start();
    }
  );
}

Listener::Listener(const std::string& address, ListenCallback accepted)
: m_accepted(accepted)
{
  sockaddr_in server_address {
    AF_INET,
    htons(6000)
  };

  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  bind(fd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address));

  listen(fd, 20);

  m_listen_io.set(fd, ev::READ);
  m_listen_io.set(this);
  m_listen_io.start();
}

void
Listener::operator()(ev::io& io, int events)
{
  sockaddr_in client_address;

  socklen_t length = sizeof(client_address);
  auto client = accept(io.fd, reinterpret_cast<sockaddr*>(&client_address), &length);

  m_accepted(client, &client_address);
}

::Promise<void, int>
Connections::read(int socket)
{
  return promise::Promise<void, int>::create(
    [=,
      io = std::make_shared<ev::io>()
    ](auto resolve, auto reject)
    {
      io->set(socket, ev::READ);
      io->set(this);
      io->start();

      m_handlers.insert({
        socket,
        {
          io, resolve, reject
        }
      });
    }
  );
}

void
Connections::operator()(ev::io& io, int events)
{
  auto iter = m_handlers.find(io.fd);

  std::get<1>(iter->second)();

  m_handlers.erase(iter);
}

::Promise<std::string, int>
readline(int socket)
{
  return Promise<std::string, int>::create(
    [socket] (auto resolve, auto reject)
    {
      auto handler = new FileIO(socket, ev::READ);
      handler->set_reader([resolve,reject](int sock) {
        char byte = 0;
        int count = 0;
        std::string result;

        while ((count = read(sock, &byte, 1)) > 0)
        {
          result += byte;

          if (byte == '\n')
          {
            break;
          }
        }

        if (count == -1 && errno != EAGAIN)
        {
          reject(errno);
        }
        else if (count == 0 && result.size() == 0)
        {
          reject(0);
        }
        else
        {
          resolve(std::move(result));
        }
      });
    }
  );
}

void
run()
{
  ev::default_loop loop;
  loop.run();
}

}
}
