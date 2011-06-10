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
  mSCSComm.signalReady();
  mSCSComm.startRead();
  
  while (true)
  {
    // Update asio to read messages from SimControl Server
    mIOService.run_one();
    
    if (mSCSComm.hasNewRun())
    {
      //cout << "(SCClient::run) new run" << endl;
      doRun(mSCSComm.getRun());
      mSCSComm.signalReady();
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
    mIOService.run_one(ec);
    if (ec)
      cout << "(SCClient::doRun) Error doing cycle: " << ec << endl;
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
  
  forceKillAgents();
  forceKillSim();
  
  mAgentAcceptor.cancel();
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
  //cout << "Spawning agent.." << endl;
  
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
  mAgentAcceptor.bind(sccendpoint);
  mAgentAcceptor.listen();
}

void SCClient::startAgentAccept()
{
  //cout << "Starting Agent accept.." << endl;
  AgentCommPtr newComm = AgentCommPtr(new AgentComm(mIOService));
  mAgentAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCClient::handleAgentAccept, this, boost::asio::placeholders::error, newComm));
}

void SCClient::handleAgentAccept(boost::system::error_code const& error, AgentCommPtr comm)
{
  //cout << "Connection accepted!" << endl;
  if (error)
    cout << "But error: " << error << endl;
  
  mAgentComms.push_back(comm);
  comm->startRead();
  
  startAgentAccept();
}
