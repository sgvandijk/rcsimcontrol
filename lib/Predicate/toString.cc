#include "predicate.hh"

using namespace sc;
using namespace std;

string Predicate::toString()
{
  std::ostringstream o;
  //  o.precision(5); //Make sure we don't place to many decimals on the output (doesn't work! (predicates are only strings :)).
  generate(o);
  return o.str();
}
