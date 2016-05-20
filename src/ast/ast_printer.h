#ifndef EAX_AST_PRINTER_H
#define EAX_AST_PRINTER_H

#include <ostream>

#include "ast_visitor.h"

namespace eax {

class AstPrinter : public AstVisitor {
public:
  AstPrinter(std::ostream& out) : out(out) {}
  
private:
  void visit(VariableExpr&) override;
  void visit(UnaryExpr&) override;
  void visit(BinaryExpr&) override;
  void visit(CallExpr&) override;
  void visit(NumberExpr&) override;
  void visit(IfExpr&) override;
  void visit(Function&) override;
  void visit(Prototype&) override;
  
private:
  std::ostream& out;
};

}

#endif
