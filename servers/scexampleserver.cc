#include "../scserver/SCServer/scserver.hh"
#include <iostream>
#include <sstream>

using namespace sc;
using namespace std;

SCServer scserver;
int cnt;

void handleAgentData(int runId, std::string const& data)
{
  cnt++;
  cout << cnt << " Agent data: " << data << endl;
  if (cnt % 20 == 0)
  {
    ostringstream out;
    out << "Cheerio chap! " << cnt;
    scserver.sendMessageToAgents(1, out.str());
    cout << "Sent: " << out.str() << endl;
  }
}

int main(int argc, char const** argv)
{
  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " <workingdir> <binary>" << endl;
    return 0;
  }
  
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
  
  scserver.run();
}
