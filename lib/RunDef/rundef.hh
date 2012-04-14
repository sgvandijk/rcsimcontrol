#ifndef SC_RUNDEF_HH_
#define SC_RUNDEF_HH_

#include <boost/smart_ptr.hpp>
#include <string>
#include <cstring>

#define SC_ARGLEN 64

namespace sc
{
  struct AgentDef
  {
    AgentDef()
      : nArgs(0), args(0)
    {
      memset(workDir, 0, SC_ARGLEN);
      memset(binary, 0, SC_ARGLEN);
    }
    
    AgentDef(std::string const& _binary,
	     std::string const& _workDir)
      : nArgs(0), args(0)
    {
      memset(workDir, 0, SC_ARGLEN);
      memset(binary, 0, SC_ARGLEN);
      memcpy(binary, _binary.c_str(), _binary.size());
      memcpy(workDir, _workDir.c_str(), _workDir.size());
    } 
	     
	     
    ~AgentDef()
    {
      for (int i = 0; i < nArgs; ++i)
        delete[] args[i];
      delete[] args;
    }
    
    /// Agent binary
    char               binary[SC_ARGLEN];

    /// Working directory
    char               workDir[SC_ARGLEN];
    
    /// Number of command line arguments
    int                nArgs;
    
    /// Command line arguments
    char**             args;

    /// Expected startup time
    double             startupTime;

  };
  
  struct RunDef
  {
    enum TerminationCondition
    {
      TC_FULLGAME,
      TC_TIMED,
      TC_AGENTTERM
    };
      
    ~RunDef() { delete[] agents; }
    
    /// Run ID
    int                   id;

    /// The termination condition
    TerminationCondition  termCond;
    
    /// Time until termination, when timed termination is activated
    double termTime;
    
    /// Number of agents
    int                   nAgents;
    
    /// Agents
    AgentDef*             agents;
    
    static RunDef* readFromBuf(char const* buf);
    static int writeToBuf(char* buf, RunDef const* runDef);
  };

  typedef boost::shared_ptr<RunDef> RunDefPtr;
}

#endif // SC_RUNDEF_HH_
