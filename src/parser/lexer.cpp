#include <cassert>
#include <sstream>
#include <llvm/ADT/STLExtras.h>

#include "lexer.h"
#include "../ast/expr.h"
#include "../ast/function.h"
#include "../ast/globals.h"
#include "../util/error.h"

using namespace eax;

Lexer::Lexer()
  : binaryOperatorPrecedence{
      {'<', 1}, {'>', 1}, {'+', 2}, {'-', 2}, {'*', 3}, {'/', 3}
    } {
}

int Lexer::getToken() {
  static int lastChar = '\n';
  
  while (std::isspace(lastChar)) {
    lastChar = readChar();
  }
  
  if (std::isalpha(lastChar)) {
    identifierValue = lastChar;
    
    while (std::isalnum(lastChar = readChar())) {
      identifierValue += lastChar;
    }
    
    if (identifierValue == "def") return TokenDef;
    return TokenIdentifier;
  }
  
  if (std::isdigit(lastChar) || lastChar == '.') {
    std::string numberStr;
    do {
      numberStr += lastChar;
      lastChar = readChar();
    } while (std::isdigit(lastChar) || lastChar == '.');
    
    numberValue = std::stod(numberStr);
    return TokenNumber;
  }
  
  if (lastChar == '#') {
    do {
      lastChar = readChar();
    } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
    
    if (lastChar != EOF) {
      return getToken();
    }
  }
  
  if (lastChar == EOF) {
    return TokenEof;
  }
  
  auto prevChar = lastChar;
  lastChar = readChar();
  return prevChar;
}

std::unique_ptr<Expr> Lexer::parseNumberExpr() {
  auto expr = llvm::make_unique<NumberExpr>(numberValue);
  nextToken(); // consume the number
  return std::move(expr);
}

std::unique_ptr<Expr> Lexer::parseParenExpr() {
  nextToken(); // consume '('
  auto value = parseExpr();
  if (!value) return nullptr;
  if (currentToken != ')') fatalError("expected ')'");
  nextToken(); // consume ')'
  return value;
}

std::unique_ptr<Expr> Lexer::parseIdentifierExpr() {
  std::string idName = std::move(identifierValue);
  
  if (nextToken() != '(') {
    // It's a variable.
    return llvm::make_unique<VariableExpr>(idName);
  }
  
  // It's a function call.
  std::vector<std::unique_ptr<Expr>> args;
  
  if (nextToken() != ')') {
    while (true) {
      if (auto arg = parseExpr()) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }
      if (currentToken == ')') break;
      if (currentToken != ',') {
        fatalError("expected ')' or ',' in argument list");
      }
      nextToken();
    }
  }
  nextToken(); // consume ')'
  return llvm::make_unique<CallExpr>(idName, std::move(args));
}

static void unknownTokenError(int token) {
  std::ostringstream message("unknown token ", std::ios::app);
  if (std::isprint(token)) {
    message << '\'' << char(token) << '\'';
  } else {
    message << token;
  }
  message << ", expected an expression";
  fatalError(message.str());
}

std::unique_ptr<Expr> Lexer::parsePrimaryExpr() {
  switch (currentToken) {
    case TokenIdentifier: return parseIdentifierExpr();
    case TokenNumber: return parseNumberExpr();
    case '(': return parseParenExpr();
    default:
      unknownTokenError(currentToken);
      return nullptr;
  }
}

std::unique_ptr<Expr> Lexer::parseExpr() {
  auto lhs = parsePrimaryExpr();
  if (!lhs) return nullptr;
  return parseBinOpRHS(0, std::move(lhs));
}

std::unique_ptr<Expr> Lexer::parseBinOpRHS(int exprPrecedence,
                                           std::unique_ptr<Expr> lhs) {
  while (true) {
    auto currentTokenPrecedence = getTokenPrecedence(currentToken);
    
    // If this is a binary operator that binds at least as tightly as
    // the current binary operator, consume it, otherwise we're done.
    if (currentTokenPrecedence < exprPrecedence) return lhs;
    
    // It's a binary operator.
    auto binOp = currentToken;
    nextToken();
    
    // Parse the primary expression after the binary operator.
    auto rhs = parsePrimaryExpr();
    if (!rhs) return nullptr;
    
    // If binOp binds less tightly with rhs than the operator after
    // rhs, let the pending operator take rhs as its lhs.
    int nextTokenPrecedence = getTokenPrecedence(currentToken);
    if (currentTokenPrecedence < nextTokenPrecedence) {
      rhs = parseBinOpRHS(currentTokenPrecedence+1, std::move(rhs));
      if (!rhs) return nullptr;
    }
    
    // Merge lhs and rhs.
    lhs = llvm::make_unique<BinaryExpr>(binOp, std::move(lhs),
                                               std::move(rhs));
  }
}

std::unique_ptr<Prototype> Lexer::parseFnPrototype() {
  if (currentToken != TokenIdentifier) {
    fatalError("expected function name in prototype");
  }
  std::string fnName = identifierValue;
  nextToken();
  
  if (currentToken != '(') {
    fatalError("expected '(' in prototype");
  }
  
  std::vector<std::string> paramNames;
  while (nextToken() == TokenIdentifier) {
    paramNames.push_back(std::move(identifierValue));
  }
  
  if (currentToken != ')') {
    fatalError("expected ')' in prototype");
  }
  nextToken();
  
  return llvm::make_unique<Prototype>(std::move(fnName),
                                      std::move(paramNames));
}

std::unique_ptr<Function> Lexer::parseFnDefinition() {
  nextToken();
  
  auto prototype = parseFnPrototype();
  if (!prototype) return nullptr;
  
  auto expr = parseExpr();
  if (!expr) return nullptr;
  
  return llvm::make_unique<Function>(std::move(prototype),
                                     std::move(expr));
}

std::unique_ptr<Function> Lexer::parseToplevelExpr() {
  if (auto expr = parseExpr()) {
    // Make an anonymous function.
    auto prototype = llvm::make_unique<Prototype>("__anon_expr",
                                                  std::vector<std::string>());
    return llvm::make_unique<Function>(std::move(prototype),
                                       std::move(expr));
  }
  return nullptr;
}

int Lexer::getTokenPrecedence(int token) const {
  auto iterator = binaryOperatorPrecedence.find(token);
  if (iterator != binaryOperatorPrecedence.end()) {
    return iterator->second;
  } else {
    return -1;
  }
}
