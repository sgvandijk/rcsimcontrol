#ifndef SC_AGENTDATACOMM_HH_
#define SC_AGENTDATACOMM_HH_

#include <boost/asio.hpp>
#include "../comm.hh"

namespace sc
{
  /** Agent Data Communication module
   */
  class AgentDataComm : public Comm
  {
  public:
    AgentDataComm(boost::asio::io_service& ioservice);
    
    /// Connect to SimControl client
    void connect();

    /// Send agent data to SimControl client
    void sendData(std::string const& data);
    
    /// Whether SimControl Server sent a new message
    bool newMessage()
    {
      bool res = mNewMessage;
      mNewMessage = false;
      return res;
    }
    
    std::string getMessage() const { return mMessage; }
    
  protected:
    bool mNewMessage;
    
    std::string mMessage;
    
    void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred);
  };
}

#endif // SC_SCSCOMM_HH_
