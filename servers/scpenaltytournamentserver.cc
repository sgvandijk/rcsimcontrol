#include "../scserver/SCServer/scserver.hh"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace sc;
using namespace std;

struct TeamDef
{
  string name;
  string workDir;
  string strikerStartScript;
  string keeperStartScript;
  int rating;
};

struct MatchData
{
  string team1;
  string team2;
  int score1;
  int score2;
};

SCServer scserver;
int cnt;

int runCnt = 0;
std::map<int, MatchData> matches;
std::string teamsFile;

void handleReady()
{
  // Read teams from files
  std::vector<TeamDef> teams;
  ifstream tin(teamsFile.c_str());
  string teamname;
  while (tin >> teamname)
  {
    TeamDef def;
    def.name = teamname;
    tin >> def.workDir;
    tin >> def.strikerStartScript;
    tin >> def.keeperStartScript;
    teams.push_back(def);
    cout << "team: " << teamname << " " << def.workDir << " " << def.strikerStartScript << " " << def.keeperStartScript << endl;
  }
  
  if (teams.size() < 2)
  {
    cout << "teams.dat contains less than 2 teams!" << endl;
    exit(-1);
  }
  
  // Select random teams
  random_shuffle(teams.begin(), teams.end());

  std::shared_ptr<RunDef> r1(new RunDef());
  runCnt++;
  r1->id = runCnt;
  r1->termCond = RunDef::TC_TIMED;
  r1->termTime = 45;
  r1->nAgents = 2;
  r1->agents = new AgentDef[r1->nAgents];
  
  // First team
  memcpy(r1->agents[0].binary, teams[0].strikerStartScript.c_str(), teams[0].strikerStartScript.size());
  memcpy(r1->agents[0].workDir, teams[0].workDir.c_str(), teams[0].workDir.size());
  r1->agents[0].startupTime = 5;
  r1->agents[0].nArgs = 2;
  r1->agents[0].args = new char*[2];
  r1->agents[0].args[0] = new char[32];
  memset(r1->agents[0].args[0], 0, 32);
  memcpy(r1->agents[0].args[0], "localhost", 9);
  r1->agents[0].args[1] = new char[32];
  memset(r1->agents[0].args[1], 0, 32);
  memcpy(r1->agents[0].args[1], teams[0].name.c_str(), teams[0].name.size());

  // Second team
  memcpy(r1->agents[1].binary, teams[1].keeperStartScript.c_str(), teams[1].keeperStartScript.size());
  memcpy(r1->agents[1].workDir, teams[1].workDir.c_str(), teams[1].workDir.size());
  r1->agents[1].startupTime = 5;
  r1->agents[1].nArgs = 2;
  r1->agents[1].args = new char*[2];
  r1->agents[1].args[0] = new char[32];
  memset(r1->agents[1].args[0], 0, 32);
  memcpy(r1->agents[1].args[0], "localhost", 9);
  r1->agents[1].args[1] = new char[32];
  memset(r1->agents[1].args[1], 0, 32);
  memcpy(r1->agents[1].args[1], teams[1].name.c_str(), teams[1].name.size());

  MatchData md;
  md.team1 = teams[0].name;
  md.team2 = teams[1].name;
  md.score1 = 0;
  md.score2 = 0;
  matches[r1->id] = md;
  
  scserver.addRun(r1);
}

void handleDone(int run)
{
  ofstream rout("results.dat", ios::app);
  MatchData md = matches[run];
  rout << md.team1 << " " << md.team2 << " " << md.score1 << " " << md.score2 << endl;
  matches.erase(run);
}

void handleScore(int run, int scoreLeft, int scoreRight)
{
  matches[run].score1 = scoreLeft;
  matches[run].score2 = scoreRight;
}

int main(int argc, char const** argv)
{
  teamsFile = "teams.dat";
  scserver.getReadySignal().connect(handleReady);
  scserver.getDoneSignal().connect(handleDone);
  scserver.getScoreSignal().connect(handleScore);
  
  if (argc == 2)
    teamsFile = argv[1];

  scserver.run();
}
