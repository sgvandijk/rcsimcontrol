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

void printConf()
{
  cout << "simdirpath: \"" << simDirPath << "\";" << endl;
  cout << "simspawncmd: \"" << simSpawnCmd << "\";" << endl;
  cout << "agentsbasedirpath: \"" << agentsBasedirPath << "\";" << endl;
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

  if (argc == 2 && string(argv[1]) == "--print-conf")
  {
    printConf();
    return 0;
  }

  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " <hostname> <port> [index]" << endl;
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
