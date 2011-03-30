#include "rcmcomm.hh"

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

RCMComm::RCMComm(boost::asio::io_service& ioservice)
: Comm(ioservice)
{
}

void RCMComm::sendMonData(string const& data)
{
  int len = prepareMsg(mOutMsgBuf, data);
  boost::asio::write(mSocket, boost::asio::buffer(mOutMsgBuf, len));
}
