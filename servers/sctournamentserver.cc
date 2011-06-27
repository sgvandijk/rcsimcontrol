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
  int rating;
};

SCServer scserver;
int cnt;

std::vector<TeamDef> teams;
int runCnt = 0;
std::string binary("start.sh");

void handleReady()
{
  cout << "Ready client..." << endl;
  boost::shared_ptr<RunDef> r1(new RunDef());
  runCnt++;
  r1->id = runCnt;
  r1->termCond = RunDef::TC_FULLGAME;
  r1->nAgents = 2;
  r1->agents = new AgentDef[2];
  
  // Select random teams
  int t1 = 1.0 * rand() / (RAND_MAX + 1.0) * teams.size();
  cout << "t1: " << t1 << endl;
  int t2 = t1;
  while (t1 == t2)
    t2 = 1.0 * rand() / (RAND_MAX + 1.0) * teams.size();

  // First team
  memcpy(r1->agents[0].binary, "./start.sh", 11);
  memcpy(r1->agents[0].workDir, teams[t1].workDir.c_str(), teams[t1].workDir.size());
  r1->agents[0].startupTime = 10;
  r1->agents[0].nArgs = 1;
  r1->agents[0].args = new char*[1];
  r1->agents[0].args[0] = new char[32];
  memcpy(r1->agents[0].args[0], "localhost", 10);

  // Second team
  memcpy(r1->agents[1].binary, "./start.sh", 11);
  memcpy(r1->agents[1].workDir, teams[t2].workDir.c_str(), teams[t2].workDir.size());
  r1->agents[1].startupTime = 10;
  r1->agents[1].nArgs = 1;
  r1->agents[1].args = new char*[1];
  r1->agents[1].args[0] = new char[32];
  memcpy(r1->agents[1].args[0], "localhost", 10);

  scserver.addRun(r1);
}

void handleDone(int run)
{
  cout << "run " << run << " is done!" << endl;
}

int main(int argc, char const** argv)
{
  
  // Read teams from files
  ifstream tin("teams.dat");
  string teamname;
  while (tin >> teamname)
  {
    TeamDef def;
    def.name = teamname;
    tin >> def.workDir;
    teams.push_back(def);
    cout << "team: " << teamname << " " << def.workDir << endl;
  }
  
  if (teams.size() < 2)
  {
    cout << "teams.dat contains less than 2 teams!" << endl;
    exit(-1);
  }
  
  scserver.getReadySignal().connect(handleReady);
  scserver.getDoneSignal().connect(handleDone);
  
  /*
  string workDir(argv[1]);
  string binary(argv[2]);

  cnt = 0;
  // Make dummy run
  unsigned nAgents = 1;
  boost::shared_ptr<RunDef> r1(new RunDef());
  r1->id = 1;
  r1->termCond = RunDef::TC_TIMED;
  r1->termTime = 20;
  r1->nAgents = nAgents;
  r1->agents = new AgentDef[nAgents];
  
  for (unsigned a = 0; a < nAgents; ++a)
  {
    memcpy(r1->agents[a].binary, binary.c_str(), binary.size());
    memcpy(r1->agents[a].workDir, workDir.c_str(), workDir.size());
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

  scserver.addRun(r1);
  scserver.getAgentMessageSignal().connect(handleAgentData);
  */
  
  scserver.run();
}