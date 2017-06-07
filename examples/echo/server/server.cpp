#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iterator>

#include <promise/net.hpp>

void
read_write(promise::net::Connections& connections, int socket);

auto
respond(int socket, promise::net::Connections& connections)
{
  return [=, &connections]() -> void {
    char buffer[1024];

    int count = 0;
    while ((count = read(socket, buffer, sizeof(buffer))) > 0)
    {
      std::copy(buffer, buffer + count, std::ostream_iterator<char>(std::cout));
      write(socket, buffer, count);
    }

    if (count == 0 || (count == -1 && errno != EAGAIN))
    {
      //abort
    }
    else
    {
      read_write(connections, socket);
    }
  };
}

void
read_write(promise::net::Connections& connections, int socket)
{
  connections.read(socket)
    ->then(respond(socket, connections));
}

int main(int argc, char** argv)
{
  promise::net::Connections connections;

  promise::net::Listener listener("127.0.0.1", [&](int socket, const sockaddr_in* address) {
    std::cout << "Got a connection from " << inet_ntoa(address->sin_addr) << std::endl;

    // Does it make sense to do:
    // auto promise = Connection::read(socket);
    // which returns a promise that guarantees there is data?
    // The problem is that the ev::io needs to be kept somewhere.
    read_write(connections, socket);
  });

  promise::net::run();

  return 0;
}
