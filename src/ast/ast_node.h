#ifndef EAX_AST_NODE_H
#define EAX_AST_NODE_H

#include "ast_visitor.h"

namespace eax {

/// Represents a node in the abstract syntax tree.
class AstNode {
public:
  virtual void accept(AstVisitor&) = 0;
};

}

#endif
