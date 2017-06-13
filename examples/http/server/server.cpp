#include <unistd.h>

#include <algorithm>
#include <unordered_set>

#include <promise/net.hpp>

namespace
{
  class TrimValue
  {
    public:

    template <typename T>
    TrimValue(const std::initializer_list<T>& list)
    : m_values(list.begin(), list.end())
    {
    }

    template <typename T,
      typename = typename std::enable_if<std::is_convertible<T, char>::value>::type>
    bool
    operator==(T&& value) const
    {
      return m_values.count(value);
    }

    private:
    std::unordered_set<char> m_values;
  };

  bool
  operator==(char c, const TrimValue& tv)
  {
    return tv.operator==(c);
  }
}

void
trim(std::string& s)
{
  s.erase(std::remove(s.begin(), s.end(), TrimValue{' ', '\t', '\r', '\n'}),
     s.end());
}

void
read_header(int socket, promise::net::Connections& connections);

auto
next_line(int socket, promise::net::Connections& connections)
{
  return [&, socket](const std::string& s)
  {
    auto line = s;
    trim(line);
    if (line.size() == 0)
    {
      char str[] = R"*(HTTP/1.1 200 OK
Cache-Control: private
Content-Type: text/html; charset=UTF-8
Content-Length: 72

<html><head><title>Hello</title></head><body>Hello World.</body></html>
)*";
      write(socket, str, sizeof(str) - 1);
      close(socket);
    }
    else
    {
      std::cout << line << std::endl;
      read_header(socket, connections);
    }
  };
}

void
read_header(int socket, promise::net::Connections& connections)
{
  connections.readline(socket)->then(next_line(socket, connections));
}

int main(int argc, char** argv)
{
  promise::net::Connections connections;
  promise::net::Listener listener("127.0.0.1",
    [&](auto socket, auto address)
    {
      read_header(socket, connections);
    });

  promise::net::run();

  return 0;
}
