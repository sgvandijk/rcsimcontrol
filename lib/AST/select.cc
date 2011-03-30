#include "ast.ih"

/**
 * \TODO: This can be optimized a very lot!!
 */
boost::shared_ptr<AST::Node> AST::Node::select(Path const &select) const
{
  NodeVector search_space;

  Path p = select;

  if (p.path.empty())
    return boost::shared_ptr<AST::Node>();

  if (p.path.front() == "/")
  {
    // Search in all predicates with value path[0] which are in
    // the top level of the tree.
    p.path.pop_front();
    if (p.path.empty())
      return (d_nodes.empty() ? boost::shared_ptr<AST::Node>() : d_nodes.front());
    else
      findAll(search_space, p.path.front());
  }
  // Search in all predicates with value path[0].
  else 
    findAllDeep(search_space,p.path.front());

  p.path.pop_front();

  for (NodeVector::iterator i = search_space.begin();
       i != search_space.end(); ++i)
    if (p.path.empty())
      return *i;
    else
      return (*i)->select(p);

  return boost::shared_ptr<AST::Node>();
}
