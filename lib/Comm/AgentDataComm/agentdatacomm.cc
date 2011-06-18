#include "agentdatacomm.hh"
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

AgentDataComm::AgentDataComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mNewMessage(false)
{
}

void AgentDataComm::connect()
{
  cout << "(AgentDataComm::connect) Trying to connect" << endl;
  Comm::connect("localhost", "15124");
  cout << "(AgentDataComm::connect) Done" << endl;
}

void AgentDataComm::sendData(string const& data)
{
  size_t len = data.size();
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(data, data.size()));
}

void AgentDataComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  if (error)
  {
    mConnected = false;
    cout << "(AgentDataComm::handleReadMessage) Error: " << error << endl;
    return;
  }
  
  mInMsgBuf[bytes_transferred] = 0;
  MsgType msgType = *reinterpret_cast<MsgType*>(mInMsgBuf);

/*  
  if (error)
    cout << "(AgentDataComm::handleReadMsg) Error reading message: " << error << endl;
  
  union {
    MsgType type;
    char chars[sizeof(MsgType)];
  } u;
  
  mInMsgBuf[bytes_transferred] = 0;
  memcpy(u.chars, mInMsgBuf, sizeof(MsgType));

  cout << "(AgentDataComm::handleReadMsg) Got something of type: " << u.type << endl;
*/
  cout << "(AgentDataComm::handleReadMsg) Got something of type: " << msgType << endl;

  switch (msgType)
  {
  case MT_AGENTMESSAGE:
    mNewMessage = true;
    mMessage = string(mInMsgBuf + sizeof(MsgType));
    break;

  default:
    cout << "(AgentDataComm::handleReadMsg) Unexpected message type: " << msgType << ", bytes transfered: " << bytes_transferred << endl << (mInMsgBuf + sizeof(MsgType)) << endl;
  }
  startRead();
}
