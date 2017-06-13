#include <unistd.h>

#include <promise/net.hpp>

void
read_header(int socket, promise::net::Connections& connections);

auto
next_line(int socket, promise::net::Connections& connections)
{
  return [&, socket](const std::string& s)
  {
    if (s.size() == 2)
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
      auto written = s;
      std::cout << written.erase(written.rfind('\r')) << std::endl;
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
