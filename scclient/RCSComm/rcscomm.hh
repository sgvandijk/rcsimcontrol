#ifndef SC_RCSCOMM_HH_
#define SC_RCSCOMM_HH_

#include <boost/asio.hpp>
#include "../../lib/Comm/comm.hh"
#include "../../lib/PortableParser/portableparser.hh"

namespace sc
{
  /** RCSSServer Communication module
   */
  class RCSComm : public Comm
  {
  public:
    enum PlayMode
    {
      PM_UNKNOWN = 0,
      PM_BEFORE_KICKOFF,

      PM_KICKOFF_LEFT,
      PM_KICKOFF_RIGHT,

      PM_PLAY_ON,

      PM_FREEKICK_LEFT,
      PM_FREEKICK_RIGHT,

      PM_GOAL_LEFT,
      PM_GOAL_RIGHT,

      PM_GOAL_KICK_LEFT,
      PM_GOAL_KICK_RIGHT,

      PM_CORNER_KICK_LEFT,
      PM_CORNER_KICK_RIGHT,

      PM_KICKIN_LEFT,
      PM_KICKIN_RIGHT,
      
      PM_GAME_OVER
    };
      
    RCSComm(boost::asio::io_service& ioservice);
    
    /// Connect to RCSSServer
    void connect(int serverPort = 3200);
  
    /// Perform kick-off
    void kickOff(std::string const& side = "Left");
    
    /// Get curret game time
    double getGameTime();
    
    /// Get current play mode
    PlayMode getPlayMode();
    
    /// Whether there is a new predicate
    bool newPred()
    {
      bool res = mNewPred;
      mNewPred = false;
      return res;
    }
    
    /// Get latest predicate
    PredicatePtr getPred() { return mPred; }
    
    /// Request a full monitor frame
    void requestFullState();
    
    /// Whether there is a new score
    bool newScore()
    {
      bool res = mNewScore;
      mNewScore = false;
      return res;
    }
    
    int getScoreLeft() const { return mScoreLeft; }
    
    int getScoreRight() const { return mScoreRight; }
    
    void getBallPos(double  ballPos[]) { memcpy(ballPos, mBallPos, 3 * sizeof(double)); }
  protected:
    void handleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred);
    
  protected:
    PortableParser mParser;
    
    bool mNewPred;
    boost::shared_ptr<Predicate> mPred;

    std::vector<std::string> mPlayModeList;
    std::map<std::string, PlayMode> mPlayModeMap;
    
    double mBallPos[3];
    
    double mGameTime;
    PlayMode mPlayMode;
    
    bool mNewScore;
    int mScoreLeft;
    int mScoreRight;
  };

  typedef boost::shared_ptr<RCSComm> RCSCommPtr;
}

#endif
