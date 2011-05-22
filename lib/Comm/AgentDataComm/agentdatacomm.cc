#include "agentdatacomm.hh"
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

AgentDataComm::AgentDataComm(boost::asio::io_service& ioservice)
: Comm(ioservice)
{
}

void AgentDataComm::connect()
{
  cout << "(AgentDataComm::conect) Trying to connect" << endl;
  Comm::connect("localhost", "15124");
  cout << "(AgentDataComm::conect) Done" << endl;
}

void AgentDataComm::sendData(string const& data)
{
  size_t len = data.size();
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(data, data.size()));
}