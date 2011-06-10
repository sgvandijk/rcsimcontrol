#ifndef SC_COMM_HH_
#define SC_COMM_HH_

#include <boost/asio.hpp>

namespace sc
{
  /** General Communication module
   */
  class Comm
  {
  public:
    enum MsgType
    {
      MT_READY,       ///< Client ready to receive new run
      MT_RUNDEF,      ///< Run definition
      MT_REQMONDATA,  ///< Request for monitor data
      MT_ENDMONDATA,  ///< Request to stop sending monitor data
      MT_MONDATA,     ///< Monitor data
      MT_AGENTDATA,   ///< Data received from agent
      MT_AGENTMESSAGE ///< Message sent to agent
    };
    
    Comm(boost::asio::io_service& ioservice);
    
    /// Connect to server
    void connect(std::string const& host, std::string const& port);
  
    /// Start asynchronous reading
    void startRead();
    
    /// Get pointer to socket
    boost::asio::ip::tcp::socket* getSocket() { return &mSocket; }
    
  protected:
    /** Prepare message to be sent
     * Prefixes with message length
     * @returns new message length
     */
    unsigned prepareMsg(char* buffer, std::string const& msg);
    
    void handleReadPref(boost::system::error_code const& error);
    
    virtual void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred) = 0;
    
  protected:
    boost::asio::ip::tcp::socket mSocket;
  
    char     mPrefBuf[4];
    char     mInMsgBuf[102400];
    char     mOutMsgBuf[102400];
  };
}

#endif
