#include "SCClient/scclient.hh"

#include <iostream>
#include <libconfig.h++>
#include <boost/asio.hpp>

using namespace sc;
using namespace std;
using namespace libconfig;

int main(int argc, char const** argv)
{
  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " <hostname> <port>" << endl;
    exit(-1);
  }
  string simDirPath = ".";
  string simSpawnCmd = "rcssserver3d";
  string teamsDirPath = "./";

  try
  {
    Config conf;
    conf.readFile("client.cfg");

    conf.lookupValue("simdirpath", simDirPath);
    conf.lookupValue("simspawncmd", simSpawnCmd);
    conf.lookupValue("teamsdirpath", teamsDirPath);
  }
  catch (...)
  {
    cerr << "Error reading client.cfg!" << endl;
  }

  try
  {
    SCClient scc(argv[1], argv[2]);
    scc.setSimDirPath(simDirPath);
    scc.setSimSpawnCmd(simSpawnCmd);
    scc.setTeamsDirPath(teamsDirPath);
    scc.run();
  }
  catch(...)
  {
    cerr << "Error running SCClient!" << endl;
  }
}
