#ifndef SC_AGENTDATACOMM_HH_
#define SC_AGENTDATACOMM_HH_

#include <boost/asio.hpp>
#include "../comm.hh"

namespace sc
{
  /** SimControl Server Communication module
   */
  class AgentDataComm : public Comm
  {
  public:
    AgentDataComm(boost::asio::io_service& ioservice);
    
    /// Send agent data to SimControl client
    void sendData(std::string const& data);
    
    void connect();
    
  protected:
    void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred) {}
  };
}

#endif // SC_SCSCOMM_HH_
