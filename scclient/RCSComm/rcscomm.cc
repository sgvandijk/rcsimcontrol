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
  mPlayMode(PM_UNKNOWN),
  mScoreLeft(0),
  mScoreRight(0)
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

void RCSComm::connect(int serverPort)
{
  char serverPortStr[6];
  sprintf(serverPortStr, "%d", serverPort);
  cout << "(RCSComm) Connecting to RC (port: " << serverPortStr << ")" << endl;
  Comm::connect("localhost", serverPortStr);
}

void RCSComm::kickOff(string const& side)
{
  string msg("(kickOff " + side + ")");
  unsigned messageLength = prepareMsg(mOutMsgBuf, msg);
  try
  {
    write(mSocket, boost::asio::buffer(mOutMsgBuf, messageLength));
  }
  catch(boost::system::system_error error)
  {
    mConnected = false;
  }
}

void RCSComm::requestFullState()
{
  string msg("(reqfullstate)");
  unsigned messageLength = prepareMsg(mOutMsgBuf, msg);
  try
  {
    write(mSocket, boost::asio::buffer(mOutMsgBuf, messageLength));
  }
  catch(boost::system::system_error error)
  {
    mConnected = false;
  }
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
  if (error)
  {
    mConnected = false;
    return;
  }
  
  mInMsgBuf[bytes_transferred] = 0;
  
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
      //cout << "(RCSComm) Have play modes: " << mPlayModeList.size() << endl;
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

  // Extract goal
  node = mPred->findDeep("score_left");
  if (node)
  {
    PredicatePtr pred = boost::shared_static_cast<Predicate>(node);
    mScoreLeft = pred->get(0)->getInt();
    mNewScore = true;
  }
  node = mPred->findDeep("score_right");
  if (node)
  {
    PredicatePtr pred = boost::shared_static_cast<Predicate>(node);
    mScoreRight = pred->get(0)->getInt();
    mNewScore = true;
  }
  /*
  node = mPred->findDeep("Ball");
  if (node)
  {
    cout << "Ball node!" << endl;
    for (AST::Node::iterator iter = node->begin(); iter != node->end(); ++iter)
      cout << boost::shared_static_cast<Predicate>(*iter)->toString() << endl;
      
    AST::NodePtr sltNode = node->find("SLT");
    if (sltNode)
    {
      cout << "Ball SLT node!" << endl;
      double m[16];
      for (unsigned i = 0; i < 16; ++i)
        m[i] = boost::shared_static_cast<Predicate>(sltNode->getChild(i))->getDouble();
      mBallPos[0] = m[3];
      mBallPos[1] = m[7];
      mBallPos[2] = m[11];
    }
  }
  * */
  
  mNewPred = true;
  
  startRead();
}
