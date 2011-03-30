#ifndef SC_SCCLIENT_HH_
#define SC_SCCLIENT_HH_

#include <string>
#include <queue>
#include <boost/asio.hpp>
#include "../../lib/RunDef/rundef.hh"
#include "../../lib/Process/process.hh"
#include "../RCSComm/rcscomm.hh"
#include "../SCSComm/scscomm.hh"
#include "../AgentComm/agentcomm.hh"

namespace sc
{
  /** SimControl Client
   * 
   * Client of the SimControl framework. This is spawned by the
   * SimControl Server, and performs runs by spawning the RCSSServer3D
   * and agents.
   */
  class SCClient
  {
  // Public methods
  public:
    /// Constructor
    SCClient(std::string const& scshost, std::string const& scsport);
    
    /// Run SimControl client
    void run();
    
  // Private methods
  private:
    /// Connect to SimControl Server
    void connectToSCS();
    
    /// Do a single run
    void doRun(boost::shared_ptr<RunDef> runDef);
    
    /// Spawn an instance of the simulator
    void spawnSim();
    
    /// Send SIGKILL to the agents
    void forceKillAgents();
    
    /// Send SIGKILL to the simulator
    void forceKillSim();
    
    /// Spawn an instance of an agent
    void spawnAgent(AgentDef const& agentDef);
    
    /// Initialize the acception of connections
    void initAcceptors();

    /// Start accepting connections from agents
    void startAgentAccept();

    /// Handle a new connection with an Agent
    void handleAgentAccept(boost::system::error_code const& error, AgentCommPtr conn);

  // Private members
  private:
    /// Main IO service
    boost::asio::io_service mIOService;

    /// SimControl Server hostname
    std::string mSCSHost; 
    
    /// SimControl Server port
    std::string mSCSPort;
    
    /// SimControl Server communication instance
    SCSComm mSCSComm;
    
    /// Simulator process
    ProcessPtr mSimProc;
    
    /// Agent processes
    std::list<ProcessPtr> mAgentProcs;

    /// Agent Connection acceptor
    boost::asio::ip::tcp::acceptor mAgentAcceptor;
    
    /// List of connections with agents
    std::list<AgentCommPtr> mAgentComms;
    
  };
}

#endif // SC_SCCLIENT_HH_
