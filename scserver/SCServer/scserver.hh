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
  typedef boost::signal<void(void)> ReadySignal;
  typedef boost::signal<void(int)> DoneSignal;
  typedef boost::signal<void(int, int, int)> ScoreSignal;
  
  class SCServer
  {
  public:
    SCServer();
    
    void run();
    
    /// Add a run to the list
    void addRun(RunDefPtr rundef) { mRuns.push_back(rundef); }
    
    /// Send a message to all agents in a certain run
    void sendMessageToAgents(int runId, std::string const& msg);

    /// Get signal to add a handler for ready clients
    ReadySignal& getReadySignal() { return mSignalReady; }
    
    /// Get signal to add a handler for ready clients
    DoneSignal& getDoneSignal() { return mSignalDone; }

    /// Get signal to add a handler for agent data
    MessageSignal& getAgentMessageSignal() { return mSignalAgentMessage; }
    
    /// Get signal to add a handler for score data
    ScoreSignal& getScoreSignal() { return mSignalScore; }
    
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
    
    /// List of runs to perform
    std::list<RunDefPtr> mRuns;
    
    /// Signal agent data
    MessageSignal mSignalAgentMessage;
    
    /// Signal that a client is ready for a new run
    ReadySignal mSignalReady;
    
    /// Signal run finished
    DoneSignal mSignalDone;
    
    /// Signal score
    ScoreSignal mSignalScore;

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
