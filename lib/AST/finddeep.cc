#include "ast.ih"

std::shared_ptr<AST::Node> AST::Node::findDeep(string const &query) const
{
  for (NodeVector::const_iterator i = d_nodes.begin();
       i != d_nodes.end(); ++i) {
    if ((*i)->match(query))
      return *i;
    else {
      std::shared_ptr<AST::Node> p = (*i)->findDeep(query);
      if (p)
	return p;
    }
  }

  // Return empty pointer if there is no match.
  return std::shared_ptr<AST::Node>();
}
