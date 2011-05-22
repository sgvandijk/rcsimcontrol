#ifndef SC_RUNDEF_HH_
#define SC_RUNDEF_HH_

#include <boost/smart_ptr.hpp>

namespace sc
{
  struct AgentDef
  {
    ~AgentDef()
    {
      for (int i = 0; i < nArgs; ++i)
        delete[] args[i];
      delete[] args;
    }
    
    /// Working directory
    char               workDir[64];
    
    /// Expected startup time
    double             startupTime;

    /// Agent binary
    char               binary[64];

    /// Number of command line arguments
    int                nArgs;
    
    /// Command line arguments
    char**             args;
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
