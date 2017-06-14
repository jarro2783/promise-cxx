#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <unordered_set>

#include <promise/net.hpp>

namespace
{
  typedef std::vector<std::string> Headers;

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

  struct Request
  {
    std::chrono::time_point<std::chrono::system_clock> start_time;
    Headers headers;
  };

  typedef std::shared_ptr<Request> RequestPtr;
}

void
trim(std::string& s)
{
  s.erase(std::remove(s.begin(), s.end(), TrimValue{' ', '\t', '\r', '\n'}),
     s.end());
}

void
read_header(int socket, promise::net::Connections& connections, RequestPtr request);

auto
next_line(int socket, promise::net::Connections& connections, RequestPtr request)
{
  return [&connections, request, socket](const std::string& s)
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
      std::chrono::duration<double> elapsed =
        std::chrono::system_clock::now() - request->start_time;
      std::cout << "Request time: " << elapsed.count() << std::endl;
      close(socket);
    }
    else
    {
      request->headers.push_back(line);
      std::cout << line << std::endl;
      read_header(socket, connections, request);
    }
  };
}

void
read_header(int socket, promise::net::Connections& connections, RequestPtr request)
{
  // This is probably really slow. Using a promise per line is incredibly
  // inefficient.
  connections.readline(socket)->then(next_line(socket, connections, request));
}

int main(int argc, char** argv)
{
  promise::net::Connections connections;
  promise::net::Listener listener("127.0.0.1",
    [&](auto socket, auto address)
    {
      RequestPtr ptr(new Request{std::chrono::system_clock::now()});
      read_header(socket, connections, ptr);
    });

  promise::net::Loop loop;
  loop.run();

  return 0;
}
