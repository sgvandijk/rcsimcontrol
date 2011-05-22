#include "comm.hh"
#include <boost/bind.hpp>
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

Comm::Comm(boost::asio::io_service& ioservice)
: mSocket(ioservice)
{
}

void Comm::connect(std::string const& host, std::string const& port)
{
  // Resolver resolves address and port
  tcp::resolver resolver(mSocket.get_io_service());
  tcp::resolver::query query(host, port);
  
  // Can return IPV4 and IPV6 enpoints
  // Loop through all until one is found that worked
  tcp::resolver::iterator endpointIter = resolver.resolve(query);
  tcp::resolver::iterator end;
  boost::system::error_code error = boost::asio::error::host_not_found;
  while (error && end != endpointIter)
  {
    mSocket.close();
    mSocket.connect(*endpointIter++, error);
  }
  
  // None worked
  if (error)
    throw boost::system::system_error(error);  
}

void Comm::startRead()
{
  //cout << "(Comm::startRead)" << endl;
  async_read(mSocket, boost::asio::buffer(mPrefBuf, 4), boost::bind(&Comm::handleReadPref, this, boost::asio::placeholders::error));
}

unsigned Comm::prepareMsg(char* buffer, string const& msg)
{
  size_t messageLength = msg.size();
  messageLength  = htonl(messageLength);
  memcpy(buffer, reinterpret_cast<unsigned char*>(&messageLength), 4);
  memcpy(buffer + 4, msg.c_str(), msg.size());
  return msg.size() + 4;
}

void Comm::handleReadPref(boost::system::error_code const& error)
{
  // Error occured, probably connection broken
  if (error)
  {
    cout << "(Comm::handleReadPref) Error reading prefix: " << error << endl;
    return;
  }
    
  size_t messageLength;
  memcpy(reinterpret_cast<char*>(&messageLength), mPrefBuf, 4);
  messageLength = ntohl(messageLength);
  //cout << "(Comm::handleReadPref) pref: " << messageLength << endl;
  
  boost::asio::socket_base::bytes_readable command(true);
  mSocket.io_control(command);
  std::size_t bytes_readable = command.get();
  //cout << "(Comm::handleReadPref) bytes readable: " << bytes_readable << endl;

  memset(mInMsgBuf, 0, 102400);
  async_read(mSocket, boost::asio::buffer(mInMsgBuf, messageLength), boost::bind(&Comm::handleReadMsg, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
