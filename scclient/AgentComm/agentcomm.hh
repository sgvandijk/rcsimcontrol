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
    
    /// Whether there is new data
    bool newData()
    {
      bool res = mNewData;
      mNewData = false;
      return res;
    }
    
    /// Get latest data
    std::string getData() { return mData; }
    
    /// Send agent message
    bool sendMessage(std::string const& msg);
    
  protected:
    virtual void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred);
    
  protected:
    bool mNewData;
    std::string mData;
  };
  
  typedef std::shared_ptr<AgentComm> AgentCommPtr;
}

#endif
