#include "ast_printer.h"
#include "expr.h"

using namespace eax;

static std::string multicharToString(int ch) {
  return {char(ch >> 24), char(ch >> 16), char(ch >> 8), char(ch)};
}

void AstPrinter::visit(VariableExpr& expr) {
  out << expr.getName();
}

void AstPrinter::visit(UnaryExpr& expr) {
  out << expr.getOp();
  expr.getOperand().accept(*this);
}

void AstPrinter::visit(BinaryExpr& expr) {
  out << "(";
  expr.getLhs().accept(*this);
  out << " " << multicharToString(expr.getOp()) << " ";
  expr.getRhs().accept(*this);
  out << ")";
}

void AstPrinter::visit(CallExpr& expr) {
  out << expr.getName() << "(";
  auto const& args = expr.getArgs();
  for (auto i = args.begin(), e = args.end(); i != e; ++i) {
    (*i)->accept(*this);
    if (i != e-1) out << ", ";
  }
  out << ")";
}

void AstPrinter::visit(NumberExpr& expr) {
  out << expr.getValue();
}

void AstPrinter::visit(IfExpr& expr) {
  out << "if ";
  expr.getCondition().accept(*this);
  out << " then ";
  expr.getThen().accept(*this);
  out << " else ";
  expr.getElse().accept(*this);
}

void AstPrinter::visit(Function& expr) {
  // TODO
}

void AstPrinter::visit(Prototype& expr) {
  // TODO
}
