#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <memory>

#include "promise/net.hpp"

namespace
{
  struct Request
  {
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::string current_line;
    std::vector<std::string> headers;
  };

  typedef std::shared_ptr<Request> RequestPtr;
}

auto
read_header(int socket, promise::net::Connections& connections, RequestPtr ptr)
{
  return [socket, &connections, ptr]()
  {
    char buffer[4096];
    int count = 0;
    bool done = false;

    while (!done && (count = recv(socket, buffer, sizeof(buffer), MSG_PEEK)) > 0)
    {
      auto newline = std::find(buffer, buffer + count, '\n');

      if (newline == buffer + count)
      {
        ptr->current_line.append(buffer, buffer + count);
      }
      else
      {
        auto line = ptr->current_line + std::string(buffer, newline);

        line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
          return c == '\n' || c == '\r';
        }), line.end());

        if (line.size() == 0)
        {
          done = true;
        }
        else
        {
          ptr->headers.push_back(line);
          ptr->current_line.clear();
        }
        count = newline - buffer + 1;
      }

      read(socket, buffer, count);
    }

    if (done)
    {
      connections.cancel(socket);
      for (auto& header : ptr->headers)
      {
        std::cout << header << std::endl;
      }

      char str[] = R"*(HTTP/1.1 200 OK
Cache-Control: private
Content-Type: text/html; charset=UTF-8
Content-Length: 72

<html><head><title>Hello</title></head><body>Hello World.</body></html>
)*";
      write(socket, str, sizeof(str) - 1);
      std::chrono::duration<double> elapsed =
        std::chrono::system_clock::now() - ptr->start_time;
      std::cout << "Request time: " << elapsed.count() << std::endl;
      close(socket);
    }
  };
}

int main(int argc, char** argv)
{
  promise::net::Connections connections;
  promise::net::Listener listener("127.0.0.1",
    [&](auto socket, auto address)
    {
      RequestPtr ptr(new Request{std::chrono::system_clock::now()});
      connections.read(socket)->then(read_header(socket, connections, ptr));
    });

  promise::net::Loop loop;
  loop.run();

  return 0;
}
