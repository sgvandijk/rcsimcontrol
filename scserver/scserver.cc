#include "SCServer/scserver.hh"
#include <iostream>

using namespace sc;
using namespace std;

void handleAgentData(int runId, std::string const& data)
{
  cout << "Agent data: " << data << endl;
}

int main()
{
  // Make dummy run
  boost::shared_ptr<RunDef> r1(new RunDef());
  r1->id = 1;
  r1->termCond = RunDef::TC_FULLGAME;
  r1->nAgents = 6;
  r1->agents = new AgentDef[6];
  
  for (unsigned a = 0; a < 6; ++a)
  {
    string binary("./boldagent");
    string workDir("/home/sander/src/boldagent.simcontrol");
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

  SCServer scserver;
  scserver.addRun(r1);
  scserver.getAgentMessageSignal().connect(handleAgentData);
  
  scserver.run();
}
