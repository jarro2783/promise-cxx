#include <unistd.h>

#include <iterator>

#include <promise/net.hpp>

promise::net::Connections connections;

std::function<void()>
write_result(int socket)
{
  return [=]() {
    char buffer[1024];
    int count = 0;
    while ((count = read(socket, buffer, sizeof(count))) > 0)
    {
      std::copy(buffer, buffer + count, std::ostream_iterator<char>(std::cout));
      std::cout << std::flush;
    }

    connections.read(socket)->then(write_result(socket));
  };
}

void
handle_connection(int socket)
{
  const char buf[] = "hello";
  auto result = write(socket, buf, sizeof(buf));

  std::cout << result << std::endl;

  if (result == -1)
  {
    perror("writing");
  }

  connections.read(socket)->then(write_result(socket));
}

void
reject_connection(int error)
{
  std::cout << "Rejected connection with " << error << std::endl;
  std::cout << strerror(error) << std::endl;
}

int main(int argc, char** argv)
{
  promise::net::NetConnection connection;

  connection.connect("127.0.0.1", 6000)
    ->then(handle_connection, reject_connection)
  ;
  promise::net::run();

  return 0;
}
