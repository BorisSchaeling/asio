#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include "asio.hpp"

class server
{
public:
  server(asio::demuxer& d, short port)
    : demuxer_(d),
      socket_(d, asio::inet_address_v4(port))
  {
    socket_.async_recvfrom(data_, max_length, sender_address_,
        boost::bind(&server::handle_recvfrom, this, _1, _2));
  }

  void handle_recvfrom(const asio::socket_error& error, size_t bytes_recvd)
  {
    if (!error && bytes_recvd > 0)
    {
      socket_.async_sendto(data_, bytes_recvd, sender_address_,
          boost::bind(&server::handle_sendto, this, _1, _2));
    }
    else
    {
      socket_.async_recvfrom(data_, max_length, sender_address_,
          boost::bind(&server::handle_recvfrom, this, _1, _2));
    }
  }

  void handle_sendto(const asio::socket_error& error, size_t bytes_sent)
  {
    socket_.async_recvfrom(data_, max_length, sender_address_,
        boost::bind(&server::handle_recvfrom, this, _1, _2));
  }

private:
  asio::demuxer& demuxer_;
  asio::dgram_socket socket_;
  asio::inet_address_v4 sender_address_;
  enum { max_length = 1024 };
  char data_[max_length];
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_udp_echo_server <port>\n";
      return 1;
    }

    asio::demuxer d;

    using namespace std; // For atoi.
    server s(d, atoi(argv[1]));

    d.run();
  }
  catch (asio::socket_error& e)
  {
    std::cerr << "Socket error: " << e.message() << "\n";
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}