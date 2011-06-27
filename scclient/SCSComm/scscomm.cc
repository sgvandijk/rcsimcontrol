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

bool SCSComm::signalReady()
{
  sendMsg(MT_READY);
}

bool SCSComm::signalDone()
{
  sendMsg(MT_RUNDONE);
}

bool SCSComm::sendMonData(string const& data)
{
  sendMsg(MT_MONDATA, data);
}

bool SCSComm::sendAgentData(string const& data)
{
  sendMsg(MT_AGENTDATA, data);
}

void SCSComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  if (error)
  {
    mConnected = false;
    return;
  }
  
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
  
  case MT_AGENTMESSAGE:
    mNewAgentMessage = true;
    mAgentMessage = string(mInMsgBuf + sizeof(msgType));
    cout << "(SCSComm::handleReadMsg) Agent message: " << mAgentMessage << endl;
    break;

  default:
    cout << "(SCSComm::handleReadMsg) Unexpected message type: " << msgType << endl;
  }
  startRead();
}
