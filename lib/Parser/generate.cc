#include "parser.ih"

string Parser::generate(std::shared_ptr<Predicate> const &pred)
{
  stringstream sstrm;
  
  pred->generate(sstrm);

  return sstrm.str();
}
