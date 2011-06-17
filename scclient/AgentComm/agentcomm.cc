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
  if (error)
  {
    mConnected = false;
    return;
  }
  
  mNewData = true;
  mData = mInMsgBuf;
  //cout << "(AgentComm::handeReadMsg) Got agent data: " << mData << endl;
  startRead();
}

bool AgentComm::sendMessage(string const& msg)
{
  bool success = sendMsg(MT_AGENTMESSAGE, msg);
  if (!success)
    mConnected = false;
  return mConnected;
}
