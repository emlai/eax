#ifndef EAX_AST_VISITOR_H
#define EAX_AST_VISITOR_H

namespace eax {

class VariableExpr;
class UnaryExpr;
class BinaryExpr;
class CallExpr;
class NumberExpr;
class BoolExpr;
class IfExpr;
class Function;
class Prototype;

class AstVisitor {
public:
  virtual void visit(VariableExpr&) = 0;
  virtual void visit(UnaryExpr&) = 0;
  virtual void visit(BinaryExpr&) = 0;
  virtual void visit(CallExpr&) = 0;
  virtual void visit(NumberExpr&) = 0;
  virtual void visit(BoolExpr&) = 0;
  virtual void visit(IfExpr&) = 0;
  virtual void visit(Function&) = 0;
  virtual void visit(Prototype&) = 0;
};

}

#endif
