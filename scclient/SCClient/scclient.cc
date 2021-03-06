#include "scclient.hh"

#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/asio/signal_set.hpp>
#include <sys/time.h>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCClient::SCClient(string const& scshost, string const& scsport, int index)
  : mSimDirPath("."),
    mSimSpawnCmd("rcssserver3d"),
    mTeamsDirPath("./"),
    mIOService(),
    mSignals(mIOService),
    mSCSHost(scshost),
    mSCSPort(scsport),
    mIndex(index),
    mSCSComm(mIOService),
    mSimProc(),
    mAgentAcceptor(mIOService)
{
  mSignals.add(SIGINT);
  mSignals.add(SIGTERM);
  mSignals.async_wait(boost::bind(&SCClient::handleSignal, this));
  mAgentPort = 3100 + mIndex - 1;
  mServerPort = 3200 + mIndex - 1;
}

void SCClient::run()
{
  // Initialize connection acceptors
  initAcceptors();

  // Connect to SimControl Server and start async reading from it
  connectToSCS();
  mSCSComm.startRead();
  
  while (true)
  {
    if (!mSCSComm.isConnected())
    {
      cout << "(SCClient) Server disconnected!" << endl;
      end();
      break;
    }

    mSCSComm.signalReady();
    bool doneRun = false;
    while (mSCSComm.isConnected() && !doneRun)
    {
      // Update asio to read messages from SimControl Server
      mIOService.run_one();
      
      if (mSCSComm.hasNewRun())
      {
	try
	{
	  doRun(mSCSComm.getRun());
	}
	catch (...)
	{
	  cout << "(SCClient) Error doing run!" << endl;
	  cleanup();
	}

        doneRun = true;
	mSCSComm.signalDone();
      }
    }
  }
}

void SCClient::cleanup()
{
  // Shutdown Agent Coms
  for (std::list<AgentCommPtr>::iterator iter = mAgentComms.begin();
       iter != mAgentComms.end();
       ++iter)
   (*iter)->shutdown();
  
  // Shutdown RCS Comm
  mRCSComm->shutdown();
  
  // Kill all agent processes
  forceKillAgents();
  
  // Kill simulator process
  forceKillSim();
  
  // Stop accepting connections from agents
  mAgentAcceptor.cancel();
  
  // Handle remaining events caused by shutdown
  mIOService.poll();
  
  mAgentComms.clear();
  
  mCurrentRun.reset();
}

void SCClient::end()
{
  cout << "(SCClient) Closing and cleaning up" << endl;

  cleanup();

  mIOService.stop();
}

void SCClient::connectToSCS()
{
  cout << "(SCClient) Connecting to SimControl server (" << mSCSHost << ":" << mSCSPort << ")... " << endl;
  mSCSComm.connect(mSCSHost, mSCSPort);
  cout << "(SCClient) Done!" << endl;
}

void SCClient::doRun(std::shared_ptr<RunDef> runDef)
{
  mCurrentRun = runDef;

  cout << "(SCClient) Starting run " << runDef->id << endl;
  cout << "(SCClient) Spawning simulator... " << endl;
  // Spawn simulator
  spawnSim();
  usleep(5e5);
  cout << "(SCClient) Done!" << endl;

  // Start listening for agents
  startAgentAccept();
  
  boost::system::error_code ec;

  // Spawn agents
  for (int i = 0; i < runDef->nAgents; ++i)
  {
    cout << "(SCClient) Spawning agent " << (i + 1) << "... " << endl;
    spawnAgent(runDef->agents[i]);

    // Sleep until agent is started (TODO: use better (boost?) timer);
    for (double s = 0; s < runDef->agents[i].startupTime; s += 0.1)
    {
      usleep(1e5);
      mIOService.poll(ec);
    }
    cout << "(SCClient) Done!" << endl;
  }

  // Connect to simulator and start async reading from it
  cout << "(SCClient) Connecting to simulator... " << endl;
  mRCSComm = RCSCommPtr(new RCSComm(mIOService));
  mRCSComm->connect(mServerPort);
  mRCSComm->startRead();
  cout << "(SCClient) Done!" << endl;

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
      cout << "(SCClient) SimControl Server disconnected!" << endl;
      break;
    }
    
    // Check if a new predicate arrived from the simulator
    if (mRCSComm->newPred())
    {
      switch(mRCSComm->getPlayMode())
      {
      // Do the kickoff
      case RCSComm::PM_BEFORE_KICKOFF:
        if (firstHalf)
          mRCSComm->kickOff("Left");
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
              mRCSComm->kickOff("Right");
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
      if (mRCSComm->newScore())
        mSCSComm.sendScore(mRCSComm->getScoreLeft(), mRCSComm->getScoreRight());
      
      // Check if time ran out if we are running in timed mode
      if (runDef->termCond == RunDef::TC_TIMED &&
	  mRCSComm->getGameTime() > runDef->termTime)
        running = false;
      
      // If we just got a new request to pass monitor info to the server
      // request a new full state frame from the simulator
      if (mSCSComm.newMonDataRequest())
        mRCSComm->requestFullState();
      else
        // Send on monitordata
        if (mSCSComm.monDataRequested())
          mSCSComm.sendMonData(mRCSComm->getPred()->toString());
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
  mRCSComm->shutdown();
  
  // Kill all agent processes
  forceKillAgents();
  
  // Kill simulator process
  forceKillSim();
  
  // Stop accepting connections from agents
  mAgentAcceptor.cancel();
  
  // Handle remaining events caused by shutdown
  mIOService.poll();
  
  mAgentComms.clear();
  
  mCurrentRun.reset();
}

void SCClient::spawnSim()
{
  for (int a = 0; a < mSimArgs.size(); ++a)
  {
    if (mSimArgs[a] == string("%ap"))
    {
      char agentPortArg[6];
      sprintf(agentPortArg, "%d", mAgentPort);
      mSimArgs[a] = agentPortArg;
    }
    if (mSimArgs[a] == string("%sp"))
    {
      char serverPortArg[6];
      sprintf(serverPortArg, "%d", mServerPort);
      mSimArgs[a] = serverPortArg;
    }

  }

  if (!mSimProc)
    mSimProc = ProcessPtr(new Process(mSimSpawnCmd, mSimDirPath, mSimArgs));
    
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
  if (mSimProc.get())
    mSimProc->forceKill();
  mSimProc.reset();
}

void SCClient::spawnAgent(AgentDef const& agentDef)
{
  vector<string> args;
  for (int i = 0; i < agentDef.nArgs; ++i)
  {
    if (agentDef.args[i] == string("%ap"))
    {
      char agentPortArg[6];
      sprintf(agentPortArg, "%d", mAgentPort);
      args.push_back(agentPortArg);
    }
    else if (agentDef.args[i] == string("%sp"))
    {
      char serverPortArg[6];
      sprintf(serverPortArg, "%d", mServerPort);
      args.push_back(serverPortArg);
    }
    else if (agentDef.args[i] == string("%scp"))
    {
      char scPortArg[6];
      sprintf(scPortArg, "%d", 15124 + mIndex - 1);
      args.push_back(scPortArg);
    }
    else
      args.push_back(agentDef.args[i]);

  }
  
  cout << mTeamsDirPath << " " << agentDef.workDir << " " << agentDef.binary << endl;
  ProcessPtr proc(new Process(agentDef.binary, mTeamsDirPath + agentDef.workDir, args));
  mAgentProcs.push_back(proc);
  proc->spawn();
}

void SCClient::initAcceptors()
{
  tcp::endpoint sccendpoint(tcp::v4(), 15124 + mIndex - 1);
  mAgentAcceptor.open(sccendpoint.protocol());
  mAgentAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  mAgentAcceptor.bind(sccendpoint);
  mAgentAcceptor.listen();
}

void SCClient::startAgentAccept()
{
  cout << "(SCClient) Starting accepting agent connections..." << endl;
  AgentCommPtr newComm = AgentCommPtr(new AgentComm(mIOService));
  mAgentAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCClient::handleAgentAccept, this, boost::asio::placeholders::error, newComm));
  cout << "(SCClient) done!" << endl;
}

void SCClient::handleAgentAccept(boost::system::error_code const& error,
				 AgentCommPtr comm)
{
  cout << "(SCClient) Agent connected" << endl;
  if (error)
    return;
  
  mAgentComms.push_back(comm);
  comm->startRead();
  
  startAgentAccept();
}

void SCClient::handleSignal()
{
  end();
  exit(0);
}
