#ifndef SC_SCSERVER_HH_
#define SC_SCSERVER_HH_

#include <boost/asio.hpp>
#include <boost/signal.hpp>
#include <list>
#include "../SCCComm/scccomm.hh"
#include "../RCMComm/rcmcomm.hh"

namespace sc
{
  typedef boost::signal<void(int, std::string const&)> MessageSignal;
  
  class SCServer
  {
  public:
    SCServer();
    
    void run();
    
    /// Add a handler for agent data
    MessageSignal& getAgentMessageSignal() { return mSignalAgentMessage; }
    
  // Private members
  private:
    /// Main IO service
    boost::asio::io_service mIOService;
    
    /// SimControl Client Connection acceptor
    boost::asio::ip::tcp::acceptor mSCCAcceptor;
    
    /// List of connections with SimControl clients
    std::list<SCCCommPtr> mSCCComms;
    
    /// RC Monitor Connection acceptor
    boost::asio::ip::tcp::acceptor mRCMAcceptor;
    
    /// List of connections with RC Monitors
    std::list<RCMCommPtr> mRCMComms;
    
    /// The client currently supplying monitor data
    SCCCommPtr mMonDataClient;
    
    /// Signal agent data
    MessageSignal mSignalAgentMessage;
    
  private:
    /// Initialize the acception of connections
    void initAcceptors();
    
    /// Start accepting connections from SimControl Clients
    void startSCCAccept();
    
    /// Start accepting connections from RC Monitors
    void startRCMAccept();
    
    /// Handle a new connection with SimControl Clients
    void handleSCCAccept(boost::system::error_code const& error, SCCCommPtr conn);

    /// Handle a new connection with RC Monitor
    void handleRCMAccept(boost::system::error_code const& error, RCMCommPtr conn);
    
  };
}

#endif // SC_SCSERVER_HH_
