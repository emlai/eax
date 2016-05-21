#include <cassert>
#include <sstream>
#include <llvm/ADT/STLExtras.h>

#include "lexer.h"
#include "../ast/expr.h"
#include "../ast/function.h"
#include "../util/error.h"
#include "../util/macros.h"

using namespace eax;

Lexer::Lexer() {
  binaryOperatorPrecedence['='] = 1;
  binaryOperatorPrecedence['=='] = 2;
  binaryOperatorPrecedence['!='] = 2;
  binaryOperatorPrecedence['<'] = 3;
  binaryOperatorPrecedence['>'] = 3;
  binaryOperatorPrecedence['<='] = 3;
  binaryOperatorPrecedence['>='] = 3;
  binaryOperatorPrecedence['+'] = 4;
  binaryOperatorPrecedence['-'] = 4;
  binaryOperatorPrecedence['*'] = 5;
  binaryOperatorPrecedence['/'] = 5;
  
  idToTokenMap["def"] = TokenDef;
  idToTokenMap["if"] = TokenIf;
  idToTokenMap["then"] = TokenThen;
  idToTokenMap["else"] = TokenElse;
  idToTokenMap["true"] = TokenTrue;
  idToTokenMap["false"] = TokenFalse;
}

int Lexer::getToken() {
  int lastChar = readChar();
  
  while (std::isblank(lastChar)) { // Skip spaces and tabs
    lastChar = readChar();
  }
  
  if (std::isalpha(lastChar)) {
    identifierValue = lastChar;
    
    while (std::isalnum(lastChar = readChar())) {
      identifierValue += lastChar;
    }
    
    unreadChar(lastChar);
    
    auto iterator = idToTokenMap.find(identifierValue);
    if (iterator != idToTokenMap.end())
      return iterator->second;
    else
      return TokenIdentifier;
  }
  
  if (std::isdigit(lastChar) || lastChar == '.') {
    std::string numberStr;
    do {
      numberStr += lastChar;
      lastChar = readChar();
    } while (std::isdigit(lastChar) || lastChar == '.');
    
    unreadChar(lastChar);
    
    numberValue = std::stod(numberStr);
    return TokenNumber;
  }
  
  if (lastChar == '=') {
    int ch = readChar();
    if (ch == '=')
      return '==';
    else
      unreadChar(ch);
  }
  
  if (lastChar == '!') {
    int ch = readChar();
    if (ch == '=')
      return '!=';
    else
      unreadChar(ch);
  }
  
  if (lastChar == '<') {
    int ch = readChar();
    if (ch == '=')
      return '<=';
    else
      unreadChar(ch);
  }
  
  if (lastChar == '>') {
    int ch = readChar();
    if (ch == '=')
      return '>=';
    else
      unreadChar(ch);
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
  
  return lastChar;
}

std::unique_ptr<Expr> Lexer::parseNumberExpr() {
  auto expr = llvm::make_unique<NumberExpr>(numberValue);
  nextToken(); // consume the number
  return std::move(expr);
}

std::unique_ptr<Expr> Lexer::parseBoolExpr() {
  auto expr = llvm::make_unique<BoolExpr>(currentToken == TokenTrue);
  nextToken(); // consume the literal
  return std::move(expr);
}

std::unique_ptr<Expr> Lexer::parseParenExpr() {
  nextToken(); // consume '('
  auto value = parseExpr();
  if (!value) return nullptr;
  if (currentToken != ')') return error("expected ')'");
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
        return error("expected ')' or ',' in argument list");
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
  error(message.str());
}

std::unique_ptr<Expr> Lexer::parsePrimaryExpr() {
  switch (currentToken) {
    case TokenIdentifier: return parseIdentifierExpr();
    case TokenNumber: return parseNumberExpr();
    case TokenTrue: eax_fallthrough;
    case TokenFalse: return parseBoolExpr();
    case TokenIf: return parseIfExpr();
    case '(': return parseParenExpr();
    default:
      unknownTokenError(currentToken);
      return nullptr;
  }
}

std::unique_ptr<Expr> Lexer::parseExpr() {
  auto lhs = parseUnaryExpr();
  if (!lhs) return nullptr;
  return parseBinOpRHS(0, std::move(lhs));
}

std::unique_ptr<Expr> Lexer::parseUnaryExpr() {
  if (currentToken < 0 || currentToken == '(' || currentToken == ',')
    return parsePrimaryExpr();
  
  int opCode = currentToken;
  nextToken();
  
  if (auto operand = parseUnaryExpr())
    return llvm::make_unique<UnaryExpr>(opCode, std::move(operand));
  
  return nullptr;
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
    
    // Parse the unary expression after the binary operator.
    auto rhs = parseUnaryExpr();
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

std::unique_ptr<Expr> Lexer::parseIfExpr() {
  nextToken(); // consume 'if'
  
  auto condition = parseExpr();
  if (!condition) return nullptr;
  
  if (currentToken == TokenThen)
    nextToken();
  
  auto thenBranch = parseExpr();
  if (!thenBranch) return nullptr;
  
  if (currentToken != TokenElse) return error("expected 'else'");
  nextToken();
  
  auto elseBranch = parseExpr();
  if (!elseBranch) return nullptr;
  
  return llvm::make_unique<IfExpr>(std::move(condition),
                                   std::move(thenBranch),
                                   std::move(elseBranch));
}

std::unique_ptr<Prototype> Lexer::parseFnPrototype() {
  if (currentToken != TokenIdentifier) {
    return error("expected function name in prototype");
  }
  std::string fnName = identifierValue;
  nextToken();
  
  if (currentToken != '(') {
    return error("expected '(' in prototype");
  }
  
  std::vector<std::string> paramNames;
  while (nextToken() == TokenIdentifier) {
    paramNames.push_back(std::move(identifierValue));
    
    if (nextToken() != ',')
      break;
  }
  
  if (currentToken != ')') {
    return error("expected ',' or ')' in parameter list");
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
