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
  //------------------------------------------------------------
  // Public methods
  public:
    /// Constructor
    SCClient(std::string const& scshost, std::string const& scsport, int index);
    
    /// Set the simulator working directory
    void setSimDirPath(std::string const& path);
    
    /// Set the command to use to spawn the simulator
    void setSimSpawnCmd(std::string const& cmd);

    /// Set simulator run time arguments
    void setSimArgs(std::vector<std::string> const& args);

    /// Set base directory where to find the agents
    void setTeamsDirPath(std::string const& teamsDirPath);

    /// Run SimControl client
    void run();

    /// End SimControl client clenly
    void end();

  //------------------------------------------------------------
  // Private methods
  private:
    /// Connect to SimControl Server
    void connectToSCS();
    
    /// Do a single run
    void doRun(std::shared_ptr<RunDef> runDef);
    
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

    /// Handle signals received from system
    void handleSignal();

    /// Cleanup all connections etc
    void cleanup();

  //------------------------------------------------------------
  // Private members
  private:

    /// Simulator directory path
    std::string mSimDirPath;

    /// Simulator spawn command
    std::string mSimSpawnCmd;

    /// Arguments to pass on to simulator
    std::vector<std::string> mSimArgs;

    /// Base directory for binaries
    std::string mTeamsDirPath;

    /// Main IO service
    boost::asio::io_service mIOService;

    /// Signal set for signal handling
    boost::asio::signal_set mSignals;

    /// SimControl Server hostname
    std::string mSCSHost; 
    
    /// SimControl Server port
    std::string mSCSPort;

    /// Index of this client
    int mIndex;

    /// Simulator's agent port
    int mAgentPort;
    
    /// Simulator's server port
    int mServerPort;

    /// SimControl Server communication instance
    SCSComm mSCSComm;
    
    /// Currently running run
    std::shared_ptr<RunDef> mCurrentRun;

    /// Simulator process
    ProcessPtr mSimProc;
    
    /// Agent processes
    std::list<ProcessPtr> mAgentProcs;

    /// Agent Connection acceptor
    boost::asio::ip::tcp::acceptor mAgentAcceptor;

    /// Connection with simulator
    RCSCommPtr mRCSComm;

    /// List of connections with agents
    std::list<AgentCommPtr> mAgentComms;
    
  };



  //------------------------------------------------------------
  // Inline methods
  inline void SCClient::setTeamsDirPath(std::string const& teamsDirPath)
  {
    mTeamsDirPath = teamsDirPath;
  }

  inline void SCClient::setSimDirPath(std::string const& path)
  {
    mSimDirPath = path;
  }

  inline void SCClient::setSimSpawnCmd(std::string const& cmd)
  {
    mSimSpawnCmd = cmd;
  }

  inline void SCClient::setSimArgs(std::vector<std::string> const& args)
  {
    mSimArgs = args;
  }
}

#endif // SC_SCCLIENT_HH_
