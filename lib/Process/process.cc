#include "process.hh"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <sys/wait.h>

using namespace sc;
using namespace std;

Process::Process(string const& fileName)
: mPID(0),
  mWorkDir("."),
  mFileName(fileName)
{
}

Process::Process(string const& fileName, std::string const& workDir)
: mPID(0),
  mWorkDir(workDir),
  mFileName(fileName)
{
}

Process::Process(string const& fileName, vector<string> const& args)
: mPID(0),
  mWorkDir("."),
  mFileName(fileName),
  mArgs(args)
{
}

Process::Process(string const& fileName, std::string const& workDir, vector<string> const& args)
: mPID(0),
  mWorkDir(workDir),
  mFileName(fileName),
  mArgs(args)
{
}

void Process::spawn()
{
  mPID = fork();
  if (!mPID)
  {
    // Set up command arguments
    char** args;
    args = new char*[mArgs.size() + 2];
  
    // First argument is filename
    args[0] = new char[mFileName.size() + 1];
    memcpy(args[0], mFileName.c_str(), mFileName.size() * sizeof(char));
    args[0][mFileName.size()] = 0;
    
    // Rest of arguments
    for (unsigned i = 0; i < mArgs.size(); ++i)
    {
      args[i + 1] = new char[mArgs[i].size() + 1];
      memcpy(args[i + 1], mArgs[i].c_str(), mArgs[i].size() * sizeof(char));
      args[i + 1][mArgs[i].size()] = 0;
    }
    
    args[mArgs.size() + 1] = 0;
    
    // Change working directory
    if (mWorkDir != string("."))
    {
      if (chdir(mWorkDir.c_str()))
        cerr << "ERROR: failed setting working directory to " << mWorkDir << endl;
    }
    
    // Redirect all output (to /dev/null for now)
    int fd;

/*
    fd = open("/dev/null", O_RDWR);

    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
    close(fd);
*/
    // Execute (p version of exec searches for filename in PATH)
    execvp(mFileName.c_str(), args);
    cerr << "ERROR: spawning " << args[0] << " failed!" << endl;
    exit(-1);
  }
}

void Process::forceKill()
{
  if (mPID)
  {
    // Kill
    kill(mPID, SIGKILL);
    // Reap
    waitpid(mPID, 0, 0);

    mPID = 0;
  }
}
