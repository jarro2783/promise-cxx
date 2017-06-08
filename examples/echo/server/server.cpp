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
  return [socket, &connections](const std::string& message) -> void {
    write(socket, message.c_str(), message.size());
    read_write(connections, socket);
  };
}

void
read_write(promise::net::Connections& connections, int socket)
{
  promise::net::readline(socket)
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
