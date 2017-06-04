#include <promise/net.hpp>

void
handle_connection(int socket)
{
}

int main(int argc, char** argv)
{
  promise::net::connect("127.0.0.1", 6000)
    ->then(handle_connection);
  promise::net::run();

  return 0;
}
