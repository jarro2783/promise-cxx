#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <promise/net.hpp>

auto
respond(int socket, promise::net::Connections& connections)
{
  return [=]() -> void {
    char buffer[1024];

    int count = 0;
    while ((count = read(socket, buffer, sizeof(buffer))) > 0)
    {
      write(socket, buffer, count);
    }
  };
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
    connections.read(socket)
      ->then(respond(socket, connections));
  });

  promise::net::run();

  return 0;
}
