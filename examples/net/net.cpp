#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <promise/net.hpp>

namespace promise
{
namespace net
{

namespace
{
  template <typename T>
  class LambdaCaller
  {
    public:

    LambdaCaller();

    template <typename F>
    LambdaCaller(F&& func)
    : m_func(func)
    {
    }

    void
    operator()(T& t, int e)
    {
      m_func(t, e);
      delete this;
    }

    private:

    std::function<void(T&, int)> m_func;
  };
}

::Promise<int, int>
connect(const std::string& address, int port)
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

      result = connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

      if (result == -1 && errno != EINPROGRESS)
      {
        close(fd);
        reject(errno);
        return;
      }

      ev::io io;

      auto caller = new LambdaCaller<ev::io>(
        [resolve, reject, fd] (ev::io& event, int events) {
          unsigned int value = 0;
          unsigned int size = sizeof(value);
          getsockopt(fd, SOL_SOCKET, SO_ERROR, &value, &size);

          if (value == 0)
          {
            resolve(fd);
          }
          else
          {
            reject(value);
          }
        }
      );

      io.set(fd, ev::WRITE);
      io.set(caller);
    }
  );
}

void
run()
{
}

}
}
