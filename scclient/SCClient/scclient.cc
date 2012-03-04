#include "scclient.hh"

#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include <sys/time.h>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCClient::SCClient(string const& scshost, string const& scsport, string const& baseDir)
: mIOService(),
  mSCSHost(scshost),
  mSCSPort(scsport),
  mBaseDir(baseDir),
  mSCSComm(mIOService),
  mSimProc(),
  mAgentAcceptor(mIOService)
{
}

void SCClient::run()
{
  initAcceptors();

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
  cout << "Connecting to SimControl server (" << mSCSHost << ":" << mSCSPort << ")... ";
  mSCSComm.connect(mSCSHost, mSCSPort);
  cout << "Done!" << endl;
}

void SCClient::doRun(boost::shared_ptr<RunDef> runDef)
{
  cout << "Starting run " << runDef->id << endl;
  cout << "Spawning simulator... ";
  // Spawn simulator
  spawnSim();
  sleep(2);
  cout << "Done!" << endl;

  // Start listening for agents
  startAgentAccept();
  
  boost::system::error_code ec;

  // Spawn agents
  for (int i = 0; i < runDef->nAgents; ++i)
  {
    cout << "Spawning agent " << (i + 1) << "... ";
    spawnAgent(runDef->agents[i]);

    // Sleep until agent is started (TODO: use better (boost) timer);
    for (double s = 0; s < runDef->agents[i].startupTime; s += 1)
    {
      sleep(1);
      mIOService.poll(ec);
    }
    cout << "Done!" << endl;
  }

  // Connect to simulator and start async reading from it
  cout << "Connecting to simulator... ";
  RCSComm rcscomm(mIOService);
  rcscomm.connect();
  rcscomm.startRead();
  cout << "Done!" << endl;

  bool running = true;
  bool firstHalf = true;
  timeval firstHalfOverTime;
  firstHalfOverTime.tv_sec = 0;
  
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
    
    // Check whether we are still connected to the simulator
    if (!mSCSComm.isConnected())
    {
      cout << "SimControl Server disconnected!" << endl;
      break;
    }
    
    // Check if a new predicate arrived from the simulator
    if (rcscomm.newPred())
    {
      switch(rcscomm.getPlayMode())
      {
      // Do the kickoff
      case RCSComm::PM_BEFORE_KICKOFF:
        if (firstHalf)
          rcscomm.kickOff("Left");
        else
        {
          if (firstHalfOverTime.tv_sec == 0)
            gettimeofday(&firstHalfOverTime, 0);
          else
          {
            timeval now;
            gettimeofday(&now, 0);
            double dt = now.tv_sec - firstHalfOverTime.tv_sec;
            if (dt > 3)
              rcscomm.kickOff("Right");
          }
        }
        break;
      
      // Game over
      case RCSComm::PM_GAME_OVER:
        running = false;
        break;
        
      case RCSComm::PM_PLAY_ON:
        firstHalf = false;
        break;

      default:
        break;
      }
      
      // Forward score to SC Server
      if (rcscomm.newScore())
        mSCSComm.sendScore(rcscomm.getScoreLeft(), rcscomm.getScoreRight());
      
      // Check if time ran out if we are running in timed mode
      if (runDef->termCond == RunDef::TC_TIMED &&
	  rcscomm.getGameTime() > runDef->termTime)
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
    for (std::list<AgentCommPtr>::iterator iter = mAgentComms.begin();
	 iter != mAgentComms.end();
	 ++iter)
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
      for (list<AgentCommPtr>::iterator iter = mAgentComms.begin();
	   iter != mAgentComms.end();
	   ++iter)
        (*iter)->sendMessage(msg);
    }
  }
  
  // Shutdown Agent Coms
  for (std::list<AgentCommPtr>::iterator iter = mAgentComms.begin();
       iter != mAgentComms.end();
       ++iter)
   (*iter)->shutdown();
  
  // Shutdown RCS Comm
  rcscomm.shutdown();
  
  // Kill all agent processes
  forceKillAgents();
  
  // Kill simulator process
  forceKillSim();
  
  // Stop accepting connections from agents
  mAgentAcceptor.cancel();
  
  // Handle remaining events caused by shutdown
  mIOService.poll();
  
  mAgentComms.clear();
  
  mSCSComm.signalDone();
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
  for (list<ProcessPtr>::iterator iter = mAgentProcs.begin();
       iter != mAgentProcs.end();
       ++iter)
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
    
  ProcessPtr proc(new Process(agentDef.binary, mBaseDir + agentDef.workDir, args));
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

void SCClient::handleAgentAccept(boost::system::error_code const& error,
				 AgentCommPtr comm)
{
  if (error)
    return;
  
  mAgentComms.push_back(comm);
  comm->startRead();
  
  startAgentAccept();
}
