#include "scclient.hh"

#include <iostream>
#include <sstream>
#include <boost/bind.hpp>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCClient::SCClient(string const& scshost, string const& scsport)
: mIOService(),
  mSCSHost(scshost),
  mSCSPort(scsport),
  mSCSComm(mIOService),
  mSimProc(),
  mAgentAcceptor(mIOService)
{
}

void SCClient::run()
{
  // Connect to SimControl Server and start async reading from it
  connectToSCS();
  mSCSComm.startRead();
  
  while (mSCSComm.isConnected())
  {
    mSCSComm.signalReady();
    bool doneRun = false;
    while (!doneRun)
    {
      // Update asio to read messages from SimControl Server
      mIOService.run_one();
      
      if (mSCSComm.hasNewRun())
      {
        doRun(mSCSComm.getRun());
        doneRun = true;
      }
    }
  }
}

void SCClient::connectToSCS()
{
  //cout << "Connecting to SimControl Server on " << mSCSHost << ":" << mSCSPort << "..." << endl;
  mSCSComm.connect(mSCSHost, mSCSPort);
  //cout << "Success!" << endl;
}

void SCClient::doRun(boost::shared_ptr<RunDef> runDef)
{
  //cout << "Starting run " << runDef->id << endl;

  // Spawn simulator
  spawnSim();
  sleep(2);
  
  // Start listening for agents
  initAcceptors();
  startAgentAccept();
  
  boost::system::error_code ec;

  // Spawn agents
  for (int i = 0; i < runDef->nAgents; ++i)
  {
    spawnAgent(runDef->agents[i]);
    // Sleep until agent is started (TODO: use better (boost) timer);
    //cout << "Sleeping " << runDef->agents[i].startupTime << "s" << endl;
    sleep(runDef->agents[i].startupTime);
  }

  // Connect to simulator and start async reading from it
  RCSComm rcscomm(mIOService);
  rcscomm.connect();
  rcscomm.startRead();

  bool running = true;
  while (running)
  {
    // Update asio to read messages from simulator, agent and SimControl Server
    // Do at least one
    mIOService.run_one(ec);
    if (ec)
      break;
    
    // And handle all others that are waiting
    mIOService.poll(ec);
    if (ec)
      break;
    
    if (!mSCSComm.isConnected())
    {
      cout << "SimControl Server disconnected!" << endl;
      break;
    }
    
    //cout << "t/pm: " << rcscomm.getGameTime() << "/" << rcscomm.getPlayMode() << endl;
    
    // Check if a new predicate arrived from the simulator
    if (rcscomm.newPred())
    {
      switch(rcscomm.getPlayMode())
      {
      // Do the kickoff
      case RCSComm::PM_BEFORE_KICKOFF:
        rcscomm.kickOff();
        break;
      
      // Game over
      case RCSComm::PM_GAME_OVER:
        running = false;
        break;
        
      default:
        break;
      }
      
      // Check if time ran out if we are running in timed mode
      if (runDef->termCond == RunDef::TC_TIMED && rcscomm.getGameTime() > runDef->termTime)
        running = false;
      
      // If we just got a new request to pass monitor info to the server
      // request a new full state frame from the simulator
      if (mSCSComm.newMonDataRequest())
        rcscomm.requestFullState();
      else
        // Send on monitordata
        if (mSCSComm.monDataRequested())
          mSCSComm.sendMonData(rcscomm.getPred()->toString());
    }
    
    //Forward any agent data to the Sim Control Server
    for (std::list<AgentCommPtr>::iterator iter = mAgentComms.begin(); iter != mAgentComms.end(); ++iter)
    {
      AgentCommPtr ac = *iter;
      if (ac->newData())
      {
        string data = ac->getData();
        mSCSComm.sendAgentData(data);
      }
    }
    
    // Forward agent message to agents
    if (mSCSComm.newAgentMessageReceived())
    {
      string msg = mSCSComm.getAgentMessage();
      for (list<AgentCommPtr>::iterator iter = mAgentComms.begin(); iter != mAgentComms.end(); ++iter)
        (*iter)->sendMessage(msg);
    }
  }
  
  // Stop accepting connections from agents
  mAgentAcceptor.close(ec);
  
  // Shutdown Agent Coms
  for (std::list<AgentCommPtr>::iterator iter = mAgentComms.begin(); iter != mAgentComms.end(); ++iter)
   (*iter)->shutdown();
  
  // Shutdown RCS Comm
  rcscomm.shutdown();
  
  // Kill all agent processes
  forceKillAgents();
  
  // Kill simulator process
  forceKillSim();
  
  // Handle remaining events caused by shutdown
  mIOService.poll();
  
  mAgentComms.clear();
}

void SCClient::spawnSim()
{
  //cout << "Spawning RCSSServer3d..." << endl;
  if (!mSimProc)
    mSimProc = ProcessPtr(new Process("rcssserver3d"));
    
  mSimProc->spawn();
}

void SCClient::forceKillAgents()
{
  for (list<ProcessPtr>::iterator iter = mAgentProcs.begin(); iter != mAgentProcs.end(); ++iter)
    (*iter)->forceKill();

  mAgentProcs.clear();
}

void SCClient::forceKillSim()
{
  mSimProc->forceKill();
  mSimProc.reset();
}

void SCClient::spawnAgent(AgentDef const& agentDef)
{
  vector<string> args;
  for (int i = 0; i < agentDef.nArgs; ++i)
    args.push_back(agentDef.args[i]);
    
  ProcessPtr proc(new Process(agentDef.binary, agentDef.workDir, args));
  mAgentProcs.push_back(proc);
  proc->spawn();
}

void SCClient::initAcceptors()
{
  tcp::endpoint sccendpoint(tcp::v4(), 15124);
  mAgentAcceptor.open(sccendpoint.protocol());
  mAgentAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  mAgentAcceptor.bind(sccendpoint);
  mAgentAcceptor.listen();
}

void SCClient::startAgentAccept()
{
  AgentCommPtr newComm = AgentCommPtr(new AgentComm(mIOService));
  mAgentAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCClient::handleAgentAccept, this, boost::asio::placeholders::error, newComm));
}

void SCClient::handleAgentAccept(boost::system::error_code const& error, AgentCommPtr comm)
{
  if (error)
    return;
  
  mAgentComms.push_back(comm);
  comm->startRead();
  
  startAgentAccept();
}
