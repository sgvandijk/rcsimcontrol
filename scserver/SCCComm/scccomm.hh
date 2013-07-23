#ifndef SC_SCCCONNECTION_HH_
#define SC_SCCCONNECTION_HH_

#include <boost/asio.hpp>
#include "../../lib/Comm/comm.hh"
#include "../../lib/RunDef/rundef.hh"

namespace sc
{
  /** SimControl Client connection
   * 
   * Maintains the state of the connection with a SimControl client at the SimControl server
   */
  class SCCComm : public Comm
  {
  public:
    SCCComm(boost::asio::io_service& ioService);
    
    /// Whether the client is ready for a new run
    bool isReady() const { return mReady; }
    
    /// Whether the client finished the previous run
    bool isDone()
    {
      bool res = mDone;
      mDone = false;
      return res;
    }
    
    /// Send the client a new run
    void sendRun(std::shared_ptr<RunDef> runDef);
  
    /// Request monitor data from the client
    void requestMonData();
    
    /// Request to end monitor data from the client
    void endMonData();
    
    /// Whether there is new monitor data
    bool newMonData()
    {
      bool res = mNewMonData;
      mNewMonData = false;
      return res;
    }
    
    /// Get monitor data
    std::string getMonData() const { return mMonData; }
    
    /// Whether there is new agent data
    bool newAgentData()
    {
      bool res = mNewAgentData;
      mNewAgentData = false;
      return res;
    }
    
    /// Get the last agent data received
    std::string getAgentData() const { return mAgentData; }
    
    /// Send a message to all agents
    void sendMessageToAgents(std::string const& msg);
    
    /// Get the description of the current run being performed
    std::shared_ptr<RunDef> getCurrentRun() const { return mCurrentRun; }
    
    /// Whether there is a new score
    bool newScore()
    {
      bool res = mNewScore;
      mNewScore = false;
      return res;
    }
    
    int getScoreLeft() const { return mScoreLeft; }
    
    int getScoreRight() const { return mScoreRight; }
    
  private:
    bool mReady;
    
    std::shared_ptr<RunDef> mCurrentRun;
    
    bool mNewMonData;
    std::string mMonData;
    
    bool mNewAgentData;
    std::string mAgentData;
    
    bool mDone;

    bool mNewScore;
    int mScoreLeft;
    int mScoreRight;
    
    virtual void handleReadMsg(boost::system::error_code const& error, std::size_t bytes_transferred);
  };

  typedef std::shared_ptr<SCCComm> SCCCommPtr;
}


#endif // SC_SCCCONNECTION_HH_
