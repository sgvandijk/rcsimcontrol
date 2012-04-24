#include "scserver.hh"
#include <boost/bind.hpp>
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

SCServer::SCServer()
  : mIOService(),
    mSignals(mIOService),
    mSCCAcceptor(mIOService),
    mRCMAcceptor(mIOService)
{
  mSignals.add(SIGINT);
  mSignals.add(SIGTERM);
  mSignals.async_wait(boost::bind(&SCServer::handleSignal, this));
}

void SCServer::run()
{
  initAcceptors();
  
  startSCCAccept();
  startRCMAccept();
  
  while (true)
  {
    //cout << "SCServer::run()" << endl;
    mIOService.run_one();
    
    // Check if we have done or ready clients
    for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
    {
      if ((*iter)->isDone())
        mSignalDone((*iter)->getCurrentRun()->id);
      
      if ((*iter)->isReady())
      {
        mSignalReady();
        if (mRuns.size() > 0)
        {
          RunDefPtr rundef = mRuns.front();
          mRuns.pop_front();
          (*iter)->sendRun(rundef);
        }
      }
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
          //cout << "(SCServer::run) Error writing to RC Monitor, closing" << endl;
          iter = mRCMComms.erase(iter);
        }
      }
    }
    
    // Signal agent data and score
    for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
    {
      if ((*iter)->newAgentData())
        mSignalAgentMessage((*iter)->getCurrentRun()->id, (*iter)->getAgentData());
      if ((*iter)->newScore())
        mSignalScore((*iter)->getCurrentRun()->id, (*iter)->getScoreLeft(), (*iter)->getScoreRight());
    }
  }
}

void SCServer::end()
{
  cout << "(SCServer) Closing and cleaning up" << endl;

  // Shutdown connections to SC clients
  for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
    (*iter)->shutdown();
  mSCCComms.clear();
  mMonDataClient.reset();
  
  // Shutdown connections to RC monitors
  for (list<RCMCommPtr>::iterator iter = mRCMComms.begin(); iter != mRCMComms.end(); ++iter)
    (*iter)->shutdown();
  mRCMComms.clear();
  
  // Stop accepting connections
  mSCCAcceptor.cancel();
  mSCCAcceptor.close();
  mRCMAcceptor.cancel();
  mRCMAcceptor.close();

  mIOService.stop();
}

void SCServer::sendMessageToAgents(int runId, string const& msg)
{
  for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
  {
    SCCCommPtr scccomm = *iter;
    if (scccomm->getCurrentRun()->id == runId)
      scccomm->sendMessageToAgents(msg);
  }
}

void SCServer::initAcceptors()
{
  tcp::endpoint sccendpoint(tcp::v4(), 15123);
  mSCCAcceptor.open(sccendpoint.protocol());
  mSCCAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  mSCCAcceptor.bind(sccendpoint);
  mSCCAcceptor.listen();

  tcp::endpoint rcmendpoint(tcp::v4(), 3300);
  mRCMAcceptor.open(rcmendpoint.protocol());
  mRCMAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  mRCMAcceptor.bind(rcmendpoint);
  mRCMAcceptor.listen();
}

void SCServer::startSCCAccept()
{
  //cout << "Starting SCC accept.." << endl;
  SCCCommPtr newComm = SCCCommPtr(new SCCComm(mIOService));
  mSCCAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCServer::handleSCCAccept, this, boost::asio::placeholders::error, newComm));
}

void SCServer::handleSCCAccept(boost::system::error_code const& error, SCCCommPtr comm)
{
  //cout << "Connection accepted!" << endl;
  if (error)
    cout << "Error: " << error << endl;
  
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
  //cout << "Starting RCM accept.." << endl;
  RCMCommPtr newComm = RCMCommPtr(new RCMComm(mIOService));
  mRCMAcceptor.async_accept(*newComm->getSocket(), boost::bind(&SCServer::handleRCMAccept, this, boost::asio::placeholders::error, newComm));
}

void SCServer::handleRCMAccept(boost::system::error_code const& error, RCMCommPtr comm)
{
  //cout << "Connection accepted!" << endl;
  if (error)
    cout << "Error: " << error << endl;
  
  mRCMComms.push_back(comm);
  comm->startRead();
  
  startRCMAccept();
}

void SCServer::handleSignal()
{
  end();
  exit(0);
}
