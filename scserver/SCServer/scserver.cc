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
    //cout << "SCServer::run()" << endl;
    mIOService.run_one();
    
    // Check if we have ready clients
    for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
      if ((*iter)->isReady() && mRuns.size() > 0)
      {
        RunDefPtr rundef = mRuns.front();
        mRuns.pop_front();
        (*iter)->sendRun(rundef);
        mRuns.push_back(rundef);
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
    
    // Signal agent data
    for (list<SCCCommPtr>::iterator iter = mSCCComms.begin(); iter != mSCCComms.end(); ++iter)
    {
      if ((*iter)->newAgentData())
      {
        mSignalAgentMessage((*iter)->getCurrentRun()->id, (*iter)->getAgentData());
      }
    }
    
  }
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
  mSCCAcceptor.bind(sccendpoint);
  mSCCAcceptor.listen();

  tcp::endpoint rcmendpoint(tcp::v4(), 3201);
  mRCMAcceptor.open(rcmendpoint.protocol());
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
