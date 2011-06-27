#include "SCClient/scclient.hh"
#include <iostream>

using namespace sc;
using namespace std;

int main(int argc, char const** argv)
{
  if (argc < 3)
  {
    cout << "Usage: " << argv[0] << " <hostname> <port>" << endl;
    exit(-1);
  }
  
  
  SCClient scc(argv[1], argv[2]);
  scc.run();
}
