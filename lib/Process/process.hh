#ifndef SC_PROCESS_HH_
#define SC_PROCESS_HH_

#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

namespace sc
{
  // Utility class to spawn and control child processes
  class Process
  {
  public:
    Process();
    Process(std::string const& fileName);
    Process(std::string const& fileName, std::string const& workDir);
    Process(std::string const& fileName, std::vector<std::string> const& args);
    Process(std::string const& fileName, std::string const& workDir, std::vector<std::string> const& args);
    
    /// Spawn this process
    void spawn();

    // Kill this process (send SIGTERM), and any possible subprocesses
    void kill();

    /// Kill this process forcefully (send SIGKILL), and any possible subprocesses
    void forceKill();
    
  private:
    pid_t mPID;
    
    std::string mWorkDir;
    
    std::string mFileName;
    
    std::vector<std::string> mArgs;
  };
  
  typedef boost::shared_ptr<Process> ProcessPtr;
}

#endif
