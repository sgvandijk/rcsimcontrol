#include "rcscomm.hh"
#include <boost/bind.hpp>
#include <iostream>

using namespace sc;
using namespace std;
using boost::asio::ip::tcp;

RCSComm::RCSComm(boost::asio::io_service& ioservice)
: Comm(ioservice),
  mNewPred(false),
  mGameTime(0),
  mPlayMode(PM_UNKNOWN)
{
  Parser::initialize();

  mPlayModeMap["BeforeKickOff"] = PM_BEFORE_KICKOFF;
  mPlayModeMap["KickOff_Left"] = PM_KICKOFF_LEFT;
  mPlayModeMap["KickOff_Right"] = PM_KICKOFF_RIGHT;
  mPlayModeMap["PlayOn"] = PM_PLAY_ON;
  mPlayModeMap["free_kick_left"] = PM_FREEKICK_LEFT;
  mPlayModeMap["free_kick_right"] = PM_FREEKICK_RIGHT;
  mPlayModeMap["Goal_Left"] = PM_GOAL_LEFT;
  mPlayModeMap["Goal_Right"] = PM_GOAL_RIGHT;
  mPlayModeMap["goal_kick_left"] = PM_GOAL_KICK_LEFT;
  mPlayModeMap["goal_kick_right"] = PM_GOAL_KICK_RIGHT;
  mPlayModeMap["corner_kick_left"] = PM_CORNER_KICK_LEFT;
  mPlayModeMap["corner_kick_right"] = PM_CORNER_KICK_RIGHT;
  mPlayModeMap["KickIn_Left"] = PM_KICKIN_LEFT;
  mPlayModeMap["KickIn_Right"] = PM_KICKIN_RIGHT;
  mPlayModeMap["GameOver"] = PM_GAME_OVER;
}

void RCSComm::connect()
{
  Comm::connect("localhost", "3200");
}

void RCSComm::kickOff()
{
  string msg("(kickOff Left)");
  unsigned messageLength = prepareMsg(mOutMsgBuf, msg);
  write(mSocket, boost::asio::buffer(mOutMsgBuf, messageLength));
}

double RCSComm::getGameTime()
{
  return mGameTime;
}

RCSComm::PlayMode RCSComm::getPlayMode()
{
  return mPlayMode;
}

void RCSComm::handleReadMsg(const boost::system::error_code& error, size_t bytes_transferred)
{
  mParser.reset();
  mParser.parse('(');
  mParser.parse(mInMsgBuf, bytes_transferred);
  mParser.parse(')');
  mPred = mParser.getPredicate();
  
  // Extract game mode lists
  if (mPlayModeList.size() == 0)
  {
    AST::NodePtr pmNode = mPred->findDeep("play_modes");
    if (pmNode)
    {
      PredicatePtr pmPred = boost::shared_static_cast<Predicate>(pmNode);
      for (Predicate::iterator iter = pmPred->begin(); iter != pmPred->end(); ++iter)
        mPlayModeList.push_back(boost::shared_static_cast<Predicate>(*iter)->getStr());
      cout << "(RCSComm) Have play modes: " << mPlayModeList.size() << endl;
    }
  }
  
  // Extract game time
  AST::NodePtr node;
  node = mPred->findDeep("time");
  if (node)
  {
    PredicatePtr pred = boost::shared_static_cast<Predicate>(node);
    mGameTime = pred->get(0)->getDouble();
  }

  // Extract play mode
  node = mPred->findDeep("play_mode");
  if (node)
  {
    PredicatePtr pred = boost::shared_static_cast<Predicate>(node);
    mPlayMode = mPlayModeMap[mPlayModeList[pred->get(0)->getInt()]];
  }

  mNewPred = true;
  
  startRead();
}
