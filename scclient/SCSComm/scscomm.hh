#ifndef SC_SCSCOMM_HH_
#define SC_SCSCOMM_HH_

#include <boost/asio.hpp>
#include "../../lib/Comm/comm.hh"
#include "../../lib/RunDef/rundef.hh"

namespace sc
{
  /** SimControl Server Communication module
   */
  class SCSComm : public Comm
  {
  public:
    SCSComm(boost::asio::io_service& ioservice);
    
    /// Check whether a new run has arrived
    bool hasNewRun();
    
    /// Get the latest run definition
    boost::shared_ptr<RunDef> getRun();
    
    /// Signal to SimControl Server that client is ready for a new run
    bool signalReady();
    
    /// Signal to SimControl Server that current run is finished
    bool signalDone();
    
    /// Whether the server requested monitor data
    bool monDataRequested() const { return mMonDataRequested; }
    
    /// Whether monitor data request is new
    bool newMonDataRequest()
    {
      bool res = mNewMonDataRequest;
      mNewMonDataRequest = false;
      return res;
    }
    
    /// Send monitor data to server
    bool sendMonData(std::string const& data);
    
    /// Send agent data to the server
    bool sendAgentData(std::string const& data);
    
    /// Whether the server sent a message for the agents
    bool newAgentMessageReceived()
    {
      bool res = mNewAgentMessage;
      mNewAgentMessage = false;
      return res;
    }
    
    /// Get agent message
    std::string getAgentMessage() const
    {
      return mAgentMessage;
    }
    
  protected:
    virtual void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred);
  
  private:
    bool mNewRun;
    
    bool mMonDataRequested;
    
    bool mNewMonDataRequest;
    
    bool mNewAgentMessage;
    
    boost::shared_ptr<RunDef> mRunDef;
    
    std::string mAgentMessage;
  };
}

#endif // SC_SCSCOMM_HH_
