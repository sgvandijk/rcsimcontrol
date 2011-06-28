#include "scccomm.hh"
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCCComm::SCCComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mReady(false)
{
}

void SCCComm::sendRun(boost::shared_ptr<RunDef> runDef)
{
  mReady = false;
  mCurrentRun = runDef;
  
  MsgType msgType = MT_RUNDEF;
  size_t len = sizeof(msgType);
  size_t buflen = RunDef::writeToBuf(mOutMsgBuf, runDef.get());
  len += buflen;
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&msgType), sizeof(msgType)));
  boost::asio::write(mSocket, boost::asio::buffer(mOutMsgBuf, buflen));
}

void SCCComm::requestMonData()
{
  MsgType msgType = MT_REQMONDATA;
  size_t len = sizeof(msgType);
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&msgType), sizeof(msgType)));
}

void SCCComm::endMonData()
{
  MsgType msgType = MT_ENDMONDATA;
  size_t len = sizeof(msgType);
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<char*>(&msgType), sizeof(msgType)));
}

void SCCComm::sendMessageToAgents(string const& msg)
{
  sendMsg(MT_AGENTMESSAGE, msg);
}

void SCCComm::handleReadMsg(boost::system::error_code const& error, size_t bytes_transferred)
{
  if (error)
    cout << "(CCComm::handleReadMsg) Error reading message: " << error << endl;
    
  MsgType msgType = *reinterpret_cast<MsgType*>(mInMsgBuf);
  mInMsgBuf[bytes_transferred] = 0;
  string msg(mInMsgBuf + sizeof(msgType));
  
  switch (msgType)
  {
  case MT_READY:
    //cout << "(SCCComm::handleReadMsg) Received ready" << endl;
    mReady = true;
    break;
  
  case MT_MONDATA:
    //cout << "(SCCComm::handleReadMsg) Received monitor data" << endl;
    mMonData = msg;
    mNewMonData = true;
    break;
  
  case MT_AGENTDATA:
    //cout << "(SCCComm:handleReadMsg) Received agent data" << endl;
    mAgentData = msg;
    mNewAgentData = true;
    break;
  
  case MT_RUNDONE:
    mDone = true;
    break;
  
  case MT_SCORE:
    mScoreLeft = *reinterpret_cast<int const*>(mInMsgBuf + sizeof(msgType));
    mScoreRight = *reinterpret_cast<int const*>(mInMsgBuf + sizeof(msgType) + sizeof(int));
    mNewScore = true;
    break;
    
  default:
    cout << "(SCCComm::handleReadMsg) Unexpected message type: " << msgType << endl;
  }
  startRead();
}
