#include "scscomm.hh"
#include <boost/bind.hpp>
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCSComm::SCSComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mNewRun(false),
  mMonDataRequested(false),
  mNewMonDataRequest(false)
{
}

bool SCSComm::hasNewRun()
{
  bool nr = mNewRun;
  mNewRun = false;
  return nr;
}

boost::shared_ptr<RunDef> SCSComm::getRun()
{
  return mRunDef;
}

void SCSComm::signalReady()
{
  MsgType msgType = MT_READY;
  size_t len = sizeof(msgType);
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&msgType), sizeof(msgType)));
}

void SCSComm::sendMonData(string const& data)
{
  MsgType msgType = MT_MONDATA;
  size_t len = sizeof(msgType) + data.size();
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&msgType), sizeof(msgType)));
  boost::asio::write(mSocket, boost::asio::buffer(data, data.size()));
}

void SCSComm::sendAgentData(string const& data)
{
  MsgType msgType = MT_AGENTDATA;
  size_t len = sizeof(msgType) + data.size();
  size_t len2 = htonl(len);
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&len2), 4));
  boost::asio::write(mSocket, boost::asio::buffer(reinterpret_cast<unsigned char*>(&msgType), sizeof(msgType)));
  boost::asio::write(mSocket, boost::asio::buffer(data, data.size()));
}

void SCSComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  if (error)
    cout << "(SCSComm::handleReadMsg) Error reading message: " << error << endl;
    
  mInMsgBuf[bytes_transferred] = 0;
  MsgType msgType = *reinterpret_cast<MsgType*>(mInMsgBuf);
  switch (msgType)
  {
  case MT_RUNDEF:
    mRunDef = boost::shared_ptr<RunDef>(RunDef::readFromBuf(mInMsgBuf + sizeof(msgType)));
    mNewRun = true;
    break;
    
  case MT_REQMONDATA:
    mMonDataRequested = mNewMonDataRequest = true;
    break;
    
  case MT_ENDMONDATA:
    mMonDataRequested = false;
    break;
    
  default:
    cout << "(SCSComm::handleReadMsg) Unexpected message type: " << msgType << endl;
  }
  startRead();
}
