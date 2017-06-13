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
    htons(12002)
  };

  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

  int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  int enable = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

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

      auto handle = [resolve, reject] (ev::io& io)
      {
        resolve();
      };

      m_reader.insert({
        socket,
        std::make_tuple(io, handle)
      });
    }
  );
}

void
Connections::operator()(ev::io& io, int events)
{
  if (events & ev::READ)
  {
    auto iter = m_reader.find(io.fd);
    auto& value = iter->second;
    std::get<1>(value)(io);
  }

  if (events & ev::WRITE)
  {
    // TODO
  }
}

namespace
{
  struct ReadlineState
  {
    size_t search = 0;
    std::string line;
  };
}

::Promise<std::string, int>
Connections::readline(int socket)
{
  return Promise<std::string, int>::create(
    [socket, this] (auto resolve, auto reject)
    {
      auto io = std::make_shared<ev::io>();
      io->set(this);
      io->set(socket, ev::READ);
      io->start();

      auto reader =
      [
        =,
        state = std::make_shared<ReadlineState>()
      ]
      (ev::io& io)
      {
        std::string delimiter = "\n";
        char buffer[1024];

        // Try to find the newline delimiter in buffer
        // and construct a string from it.
        // Since we have peeked the data, we then need to actually read
        // it once we decide that we can keep it.
        int count = 0;
        while ((count = recv(io.fd, buffer, sizeof(buffer), MSG_PEEK)) > 0)
        {
          int i = 0;
          while (i < count)
          {
            if (buffer[i] == delimiter[state->search])
            {
              ++state->search;
              ++i;
            }
            else if (state->search == 0)
            {
              ++i;
            }
            else
            {
              state->search = 0;
            }

            if (state->search == delimiter.length())
            {
              recv(io.fd, buffer, i, 0);
              state->line.append(buffer, buffer + i);
              resolve(state->line);
              m_reader.erase(socket);
              return;
            }
          }

          state->line.append(buffer, buffer + count);
          recv(io.fd, buffer, count, 0);
        }
      };

      m_reader.insert(std::make_pair(
        socket,
        std::make_tuple(io, reader)
      ));
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
