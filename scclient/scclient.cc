#include "SCClient/scclient.hh"

#include <iostream>
#include <libconfig.h++>
#include <boost/asio.hpp>

using namespace sc;
using namespace std;
using namespace libconfig;

string simDirPath;
string simSpawnCmd;
string agentsBasedirPath;
vector<string> simArgs;

void printUsage()
{
  cout << "Usage: scclient hostanem port [index]" << endl
       << "       scclient --print-conf" << endl
       << endl
       << "Options:" << endl
       << "  --print-conf  Output base configuration; redirect to client.cfg and edit to change default configuration" << endl;
}

void printConf()
{
  cout << "# Working directory for running simulator spawn command" << endl;
  cout << "simdirpath: \"" << simDirPath << "\";" << endl << endl;
  cout << "# Command used to spawn simulator" << endl;
  cout << "simspawncmd: \"" << simSpawnCmd << "\";" << endl << endl;
  cout << "# Base directory of agent binaries. Clients will append team specific paths to this path" << endl;
  cout << "agentsbasedirpath: \"" << agentsBasedirPath << "\";" << endl << endl;
  cout << "# List of command arguments given to the simulator" << endl;
  cout << "simargs: [";
  if (simArgs.size() > 0)
  {
    cout << "\"" << simArgs[0] << "\"";
    for (int i = 1; i < simArgs.size(); ++i)
      cout << ", \"" << simArgs[i] << "\"";
  }
  cout << "];" << endl;
}

int main(int argc, char const** argv)
{
  simDirPath = ".";
  simSpawnCmd = "rcssserver3d";
  agentsBasedirPath = "./";

  if (argc == 2)
  {
    if (string(argv[1]) == "--help")
    {
      printUsage();
      return 0;
    }
    else if (string(argv[1]) == "--print-conf")
    {
      printConf();
      return 0;
    }
  }

  if (argc < 3)
  {
    printUsage();
    return -1;
  }

  vector<string> simArgs;
  try
  {
    Config conf;
    conf.readFile("client.cfg");

    conf.lookupValue("simdirpath", simDirPath);
    conf.lookupValue("simspawncmd", simSpawnCmd);
    conf.lookupValue("agentsbasedirpath", agentsBasedirPath);

    
    Setting& argsSetting = conf.lookup("simargs");
    for (int i = 0; i < argsSetting.getLength(); ++i)
      simArgs.push_back(argsSetting[i]);
  }
  catch (...)
  {
    cerr << "Error reading client.cfg!" << endl;
  }

  int idx = argc == 4 ? atoi(argv[3]) : 1;

  SCClient scc(argv[1], argv[2], idx);
  scc.setSimDirPath(simDirPath);
  scc.setSimSpawnCmd(simSpawnCmd);
  scc.setSimArgs(simArgs);
  scc.setTeamsDirPath(agentsBasedirPath);
  scc.run();
}
