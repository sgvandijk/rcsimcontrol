#ifndef SC_RCMCOMM
#define SC_RCMCOMM

#include "../../lib/Comm/comm.hh"

namespace sc
{
  /** RC Monitor Communication model
   * Receives connections from monitors
   */
  class RCMComm : public Comm
  {
  public:
    RCMComm(boost::asio::io_service& ioService);
    
    /// Send monitor data to monitor
    void sendMonData(std::string const& data);
    
  private:
    
    virtual void handleReadMsg(boost::system::error_code const& error, std::size_t bytes_transferred) {}
    
  };
  
  typedef std::shared_ptr<RCMComm> RCMCommPtr;
}


#endif
