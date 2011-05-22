#ifndef SC_AGENTCOMM_HH_
#define SC_AGENTCOMM_HH_

#include <boost/asio.hpp>
#include "../../lib/Comm/comm.hh"

namespace sc
{
  /** Aget Communication module
   */
  class AgentComm : public Comm
  {
  public:
    AgentComm(boost::asio::io_service& ioservice);
    
    /// Whether there is a new message
    bool newMessage()
    {
      bool res = mNewMessage;
      mNewMessage = false;
      return res;
    }
    
    /// Get latest message
    std::string getMessage() { return mMessage; }
    
  protected:
    virtual void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred);
    
  protected:
    bool mNewMessage;
    std::string mMessage;
  };
  
  typedef boost::shared_ptr<AgentComm> AgentCommPtr;
}

#endif
