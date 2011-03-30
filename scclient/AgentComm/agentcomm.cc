#include "agentcomm.hh"

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

AgentComm::AgentComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mNewMessage(false),
  mMessage("")
{
}

void AgentComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  mNewMessage = true;
  mMessage = mInMsgBuf;
}
