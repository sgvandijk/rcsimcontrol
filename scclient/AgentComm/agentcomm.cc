#include "agentcomm.hh"
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

AgentComm::AgentComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mNewData(false),
  mData("")
{
}

void AgentComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  mNewData = true;
  mData = mInMsgBuf;
  //cout << "(AgentComm::handeReadMsg) Got agent data: " << mData << endl;
  startRead();
}

void AgentComm::sendMessage(string const& msg)
{
  MsgType msgType = MT_AGENTMESSAGE;
  size_t len = sizeof(msgType) + msg.size();
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&msgType), sizeof(msgType)));
  boost::asio::write(mSocket, boost::asio::buffer(msg, msg.size()));
}
