/**********************************************************************
 * SC Tournament Server
 *
 * Copyright (C) 2012 by Sander van Dijk (sgvandijk@gmail.com)
 *
 * Usage:
 *  sctournamentserver [-cm]
 *
 * A list of teams to use in the tournament is read from a file
 * 'teams.dat' in the current working directory. The file should
 * contain at least 2 teams, with each team be on a seperate line,
 * having the following format:
 *
 * TEAMNAME  WORKINGDIR
 *
 * For each match, 2 random teams are selected to play against each
 * other. In 'challenger mode', enabled with the flag -cm, the first
 * team in the file is placed against a random team from the rest of
 * the list in each match.
 **/
//------------------------------------------------------------
#include "../scserver/SCServer/scserver.hh"
#include <libconfig.h++>
#include <iostream>
#include <sstream>
#include <fstream>

//------------------------------------------------------------
using namespace sc;
using namespace std;
using namespace libconfig;

//------------------------------------------------------------
/// Team definition
struct TeamDef
{
  string name;
  string workDir;
  int rating;
};

/// Match-data container
struct MatchData
{
  string team1;
  string team2;
  int score1;
  int score2;
};

/// The server object
SCServer scserver;

/// Number of runs done
int runCnt = 0;

/// The command to be run to start up a team
std::string binary("start.sh");

/// All match data is collected here
std::map<int, MatchData> matches;

/// Whether the server should run in challenger mode
bool challengerMode;


//------------------------------------------------------------
// Called when client signals being ready for a new run
void handleReady()
{
  // Read teams from file
  std::vector<TeamDef> teams;
  try
  {
    Config conf;
    conf.readFile("teams.cfg");

    Setting& teamsList = conf.lookup("teams");
    for (int i = 0; i < teamsList.getLength(); ++i)
    {
      TeamDef def;
      bool success = 
	teamsList[i].lookupValue("name", def.name) &&
	teamsList[i].lookupValue("workdir", def.workDir);

      if (success)
      {
	teams.push_back(def);
	cout << "team: " << def.name << " " << def.workDir << endl;
      }
      else
	cerr << "Failed reading team " << (i + 1) << endl;
    }
  }
  catch (FileIOException fexc)
  {
    cout << "Error reading configuration file (teams.cfg doesn't exist?): " << fexc.what() <<endl;
    scserver.end();
    exit(-1);
  }
  catch (ParseException pexc)
  {
    cout << "Error parsing configuration file: " << pexc.what() << endl;
    scserver.end();
    exit(-1);
  }
  catch (SettingException sexc)
  {
    cout << "Error reading configuration setting: " << sexc.getPath() << endl;
    scserver.end();
    exit(-1);
  }
  catch (ConfigException cexc)
  {
    cerr << "Error reading teams.cfg: " << cexc.what() << endl;
    scserver.end();
    exit(-1);
  }

  // Need more than 1 team for a tournament
  if (teams.size() < 2)
  {
    cout << "teams.dat contains less than 2 teams!" << endl;
    scserver.end();
    exit(-1);
  }

  TeamDef t1, t2;

  // In challenger mode, always select the first team and a random
  // team from the rest
  if (challengerMode)
  {
    t1 = teams[0];
    
    vector<TeamDef> teams2;
    for (int i = 1; i < teams.size(); ++i)
      teams2.push_back(teams[i]);
      
    random_shuffle(teams2.begin(), teams2.end());

    t2 = teams2[0];
    
  }
  // Otherwise, select two random teams
  else
  {
    // Select random teams
    random_shuffle(teams.begin(), teams.end());

    t1 = teams[0];
    t2 = teams[1];
  }
  
  // Create a run definition for a full game
  boost::shared_ptr<RunDef> r1(new RunDef());
  runCnt++;
  r1->id = runCnt;
  r1->termCond = RunDef::TC_FULLGAME;
  r1->nAgents = 2;
  r1->agents = new AgentDef[r1->nAgents];
    
  // First team
  r1->agents[0] = AgentDef("./start.sh", t1.workDir);
  r1->agents[0].startupTime = 20;
  r1->agents[0].nArgs = 2;
  r1->agents[0].args = new char*[2];
  // arg1: host
  r1->agents[0].args[0] = new char[32];
  memset(r1->agents[0].args[0], 0, 32);
  memcpy(r1->agents[0].args[0], "127.0.0.1", 9);
  // arg2: team name
  r1->agents[0].args[1] = new char[32];
  memset(r1->agents[0].args[1], 0, 32);
  memcpy(r1->agents[0].args[1], t1.name.c_str(), min((int)t1.name.size(), 32));

  // Second team
  r1->agents[1] = AgentDef("./start.sh", t2.workDir);
  r1->agents[1].startupTime = 20;
  r1->agents[1].nArgs = 2;
  r1->agents[1].args = new char*[2];
  // arg1: host
  r1->agents[1].args[0] = new char[32];
  memset(r1->agents[1].args[0], 0, 32);
  memcpy(r1->agents[1].args[0], "127.0.0.1", 9);
  // arg2: team name
  r1->agents[1].args[1] = new char[32];
  memset(r1->agents[1].args[1], 0, 32);
  memcpy(r1->agents[1].args[1], t2.name.c_str(), min((int)t2.name.size(), 32));

  // Initialize match data
  MatchData md;
  md.team1 = t1.name;
  md.team2 = t2.name;
  md.score1 = 0;
  md.score2 = 0;
  matches[r1->id] = md;
  
  // Add run to server
  scserver.addRun(r1);
}

//------------------------------------------------------------
// Called when run is finished
void handleDone(int run)
{
  // Get match data and append to results file
  ofstream rout("results.dat", ios::app);
  MatchData md = matches[run];
  rout << md.team1 << " " << md.team2 << " " << md.score1 << " " << md.score2 << endl;
  matches.erase(run);
}


//------------------------------------------------------------
// Called when score has changed
void handleScore(int run, int scoreLeft, int scoreRight)
{
  matches[run].score1 = scoreLeft;
  matches[run].score2 = scoreRight;
}

//------------------------------------------------------------
// Main
int main(int argc, char const** argv)
{
  scserver.getReadySignal().connect(handleReady);
  scserver.getDoneSignal().connect(handleDone);
  scserver.getScoreSignal().connect(handleScore);
  
  challengerMode = false;
  if (argc == 2 && argv[1] == "-cm")
  {
    cout << "Running in Challenger Mode" << endl;
    challengerMode = true;
  }

  scserver.run();

  cout << "Finished run(?)" << endl;
  return 0;
}
