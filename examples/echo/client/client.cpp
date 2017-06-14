#include <fcntl.h>
#include <unistd.h>

#include <iterator>

#include <promise/net.hpp>

promise::net::Connections connections;

void
do_echo(int);

void
do_read(int socket);

std::function<void()>
write_result(int socket)
{
  return [=]() {
    char buffer[1024];
    int count = 0;
    while ((count = read(socket, buffer, sizeof(buffer))) > 0)
    {
      std::copy(buffer, buffer + count, std::ostream_iterator<char>(std::cout));
      std::cout << std::flush;
    }

    if (count == 0 || (count == -1 && errno != EAGAIN))
    {
    }
    else
    {
      do_read(socket);
    }
  };
}

::Promise<void, int>
read_input()
{
  return connections.read(0);
}

auto
copy_input(int socket)
{
  return [=]() {
    char buffer[1024];
    int count = 0;
    while ((count = read(0, buffer, sizeof(buffer))) > 0)
    {
      write(socket, buffer, count);
    }

    if (count == 0 || (count == -1 && errno != EAGAIN))
    {
      connections.shutdown();
    }
    else
    {
      do_echo(socket);
    }
  };
}

void
do_echo(int socket)
{
  read_input()->then(copy_input(socket));
}

void
do_read(int socket)
{
  connections.read(socket)->then(write_result(socket));
}

void
handle_connection(int socket)
{
  do_echo(socket);
  do_read(socket);
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

  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  promise::net::Loop loop;
  loop.run();

  return 0;
}
