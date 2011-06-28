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
      MT_READY,         ///< Client ready to receive new run
      MT_RUNDONE,       ///< Run is finished
      MT_RUNDEF,        ///< Run definition
      MT_REQMONDATA,    ///< Request for monitor data
      MT_ENDMONDATA,    ///< Request to stop sending monitor data
      MT_MONDATA,       ///< Monitor data
      MT_AGENTDATA,     ///< Data received from agent
      MT_AGENTMESSAGE,  ///< Message sent to agent
      MT_SCORE          ///< Current score
    };
    
    Comm(boost::asio::io_service& ioservice);
    
    virtual ~Comm();
    
    /// Connect to server
    void connect(std::string const& host, std::string const& port);
  
    /// Shutdown socket
    void shutdown();
    
    /// Start asynchronous reading
    void startRead();
    
    /// Get pointer to socket
    boost::asio::ip::tcp::socket* getSocket() { return &mSocket; }
    
    /// Get (an estimate) whether this Comm module is connected
    bool isConnected() const { return mConnected; }
    
  protected:
    /** Prepare message to be sent
     * Prefixes with message length
     * @returns new message length
     */
    unsigned prepareMsg(char* buffer, std::string const& msg);
    
    void handleReadPref(boost::system::error_code const& error);
    
    virtual void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred) = 0;
    
    /// Send an empty message of type @type
    bool sendMsg(MsgType type);
    
    /// Send a message
    bool sendMsg(MsgType type, std::string const& msg);
    
    /// Send a message
    bool sendMsg(MsgType type, char* msg, int len);
    
  protected:
    boost::asio::ip::tcp::socket mSocket;
    bool mConnected;
  
    char     mPrefBuf[4];
    char     mInMsgBuf[1048576];
    char     mOutMsgBuf[1048576];
  };
}

#endif
