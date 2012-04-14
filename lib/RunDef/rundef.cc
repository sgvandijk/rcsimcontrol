#include "rundef.hh"

#include <cstring>
#include <iostream>

using namespace sc;
using namespace std;

RunDef* RunDef::readFromBuf(char const* buf)
{
  RunDef* runDef = new RunDef();
  int cursor = 0;
  memcpy(runDef, buf, sizeof(RunDef));
  cursor += sizeof(RunDef);
  
  cout << runDef->id << " " << runDef->termCond << " " << runDef->termTime << " " << runDef->nAgents << endl;
  runDef->agents = new AgentDef[runDef->nAgents];
  for (int i = 0; i < runDef->nAgents; ++i)
  {
    memcpy(runDef->agents + i, buf + cursor, sizeof(AgentDef));
    cursor += sizeof(AgentDef);
    runDef->agents[i].args = new char*[runDef->agents[i].nArgs];
    for (int j = 0; j < runDef->agents[i].nArgs; ++j)
    {
      runDef->agents[i].args[j] = new char[SC_ARGLEN];
      memcpy(runDef->agents[i].args[j], buf + cursor, SC_ARGLEN);
      cursor += SC_ARGLEN;
    }
  }
  return runDef;
}

int RunDef::writeToBuf(char* buf, RunDef const* runDef)
{
  int cursor = 0;
  memcpy(buf, runDef, sizeof(RunDef));
  cursor += sizeof(RunDef);
  for (int i = 0; i < runDef->nAgents; ++i)
  {
    memcpy(buf + cursor, runDef->agents + i, sizeof(AgentDef));
    cursor += sizeof(AgentDef);
    for (int j = 0; j < runDef->agents[i].nArgs; ++j)
    {
      memcpy(buf + cursor, runDef->agents[i].args[j], SC_ARGLEN);
      cursor += SC_ARGLEN;
    }
  }
  return cursor;
}
