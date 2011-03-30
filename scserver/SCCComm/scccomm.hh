#ifndef SC_SCCCONNECTION_HH_
#define SC_SCCCONNECTION_HH_

#include <boost/asio.hpp>
#include "../../lib/Comm/comm.hh"
#include "../../lib/RunDef/rundef.hh"

namespace sc
{
  /** SimControl Client connection
   * 
   * Maintains the state of the connection with a SimControl client at the SimControl server
   */
  class SCCComm : public Comm
  {
  public:
    SCCComm(boost::asio::io_service& ioService);
    
    /// Whether the client is ready for a new run
    bool isReady() const { return mReady; }
    
    /// Send the client a new run
    void sendRun(boost::shared_ptr<RunDef> runDef);
  
    /// Request monitor data from the client
    void requestMonData();
    
    /// Request to end monitor data from the client
    void endMonData();
    
    /// Whether there is new monitor data
    bool newMonData()
    {
      bool res = mNewMonData;
      mNewMonData = false;
      return res;
    }
    
    /// Get monitor data
    std::string getMonData() { return mMonData; }
  private:
    bool mReady;
    
    bool mNewMonData;
    
    std::string mMonData;
    
    virtual void handleReadMsg(boost::system::error_code const& error, std::size_t bytes_transferred);
  };

  typedef boost::shared_ptr<SCCComm> SCCCommPtr;
}


#endif // SC_SCCCONNECTION_HH_
