#include "parser.ih"

string Parser::generate(boost::shared_ptr<Predicate> const &pred)
{
  stringstream sstrm;
  
  pred->generate(sstrm);

  return sstrm.str();
}
