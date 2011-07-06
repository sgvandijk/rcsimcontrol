#include "SCClient/scclient.hh"
#include <iostream>

using namespace sc;
using namespace std;

int main(int argc, char const** argv)
{
  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " <hostname> <port> [basedir]" << endl;
    exit(-1);
  }
  
  string baseDir = "./";
  if (argc == 4)
    baseDir = argv[3];
    
  SCClient scc(argv[1], argv[2], baseDir);
  scc.run();
}
