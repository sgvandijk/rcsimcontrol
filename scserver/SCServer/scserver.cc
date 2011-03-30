#include "scserver.hh"
#include <boost/bind.hpp>
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCServer::SCServer()
: mSCCAcceptor(mIOService),
  mRCMAcceptor(mIOService)
{
}

void SCServer::run()
{
  initAcceptors();
  
  startSCCAccept();
  startRCMAccept();
  
  while (true)
  {
    cout << "SCServer::run()" << endl;
    mIOService.run_one();
    
    // Check if we have ready clients
    for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
      if ((*iter)->isReady())
      {
        // Make dummy run
        boost::shared_ptr<RunDef> r1(new RunDef());
        r1->id = 1;
        r1->termCond = RunDef::TC_FULLGAME;
        r1->nAgents = 6;
        r1->agents = new AgentDef[6];
        
        for (unsigned a = 0; a < 6; ++a)
        {
          memcpy(r1->agents[a].binary, "./boldagent", 11);
          memcpy(r1->agents[a].workDir, "/home/sander/src/boldagent", 26);
          r1->agents[a].startupTime = 1;
          r1->agents[a].nArgs = 2;
          r1->agents[a].args = new char*[2];
          r1->agents[a].args[0] = new char[32];
          r1->agents[a].args[1] = new char[32];
          
          memcpy(r1->agents[a].args[0], "-u", 2);
          ostringstream unum;
          unum << (a + 1);
          memcpy(r1->agents[a].args[1], unum.str().c_str(), unum.str().size());
        }
        (*iter)->sendRun(r1);
      }
    
    // Forward monitor data
    if (mMonDataClient.get() && mMonDataClient->newMonData())
    {
      string data = mMonDataClient->getMonData();
      for (list<RCMCommPtr>::iterator iter = mRCMComms.begin(); iter != mRCMComms.end();)
      {
        try
        {
          (*iter)->sendMonData(data);
          ++iter;
        }
        // Error writing to monitor. Assume it's dead and remove from list
        catch (boost::system::system_error er)
        {
          cout << "(SCServer::run) Error writing to RC Monitor, closing" << endl;
          iter = mRCMComms.erase(iter);
        }
      }
    }
  }
}

void SCServer::initAcceptors()
{
  tcp::endpoint sccendpoint(tcp::v4(), 15123);
  mSCCAcceptor.open(sccendpoint.protocol());
  mSCCAcceptor.bind(sccendpoint);
  mSCCAcceptor.listen();

  tcp::endpoint rcmendpoint(tcp::v4(), 3201);
  mRCMAcceptor.open(rcmendpoint.protocol());
  mRCMAcceptor.bind(rcmendpoint);
  mRCMAcceptor.listen();
}

void SCServer::startSCCAccept()
{
  cout << "Starting SCC accept.." << endl;
  SCCCommPtr newComm = SCCCommPtr(new SCCComm(mIOService));
  mSCCAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCServer::handleSCCAccept, this, boost::asio::placeholders::error, newComm));
}

void SCServer::handleSCCAccept(boost::system::error_code const& error, SCCCommPtr comm)
{
  cout << "Connection accepted!" << endl;
  if (error)
    cout << "But error: " << error << endl;
  
  mSCCComms.push_back(comm);
  
  if (mMonDataClient.get())
    mMonDataClient->endMonData();
  
  mMonDataClient = comm;
  comm->requestMonData();
  
  comm->startRead();
  
  startSCCAccept();
}

void SCServer::startRCMAccept()
{
  cout << "Starting RCM accept.." << endl;
  RCMCommPtr newComm = RCMCommPtr(new RCMComm(mIOService));
  mRCMAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCServer::handleRCMAccept, this, boost::asio::placeholders::error, newComm));
}

void SCServer::handleRCMAccept(boost::system::error_code const& error, RCMCommPtr comm)
{
  cout << "Connection accepted!" << endl;
  if (error)
    cout << "But error: " << error << endl;
  
  mRCMComms.push_back(comm);
  comm->startRead();
  
  startRCMAccept();
}
