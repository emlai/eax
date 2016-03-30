#ifndef EAX_LEXER_H
#define EAX_LEXER_H

#include <climits>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <llvm/ADT/StringRef.h>

#include "../ast/expr.h"

namespace eax {

class Prototype;
class Function;

enum Token {
  TokenEof = -1,
  TokenDef = -2,
  TokenIdentifier = -3,
  TokenNumber = -4,
  TokenIf = -5,
  TokenThen = -6,
  TokenElse = -7,
  TokenTrue = -8,
  TokenFalse = -9
};

class Lexer {
public:
  Lexer();
  
  /// Updates CurrentToken with the next token and returns it.
  int nextToken() { return currentToken = getToken(); }
  
  std::unique_ptr<Function> parseToplevelExpr();
  std::unique_ptr<Function> parseFnDefinition();
  
private:
  /// Returns the next token from the input source.
  int getToken();
  
  /// Returns the next character from the input source.
  int readChar() const { return std::getchar(); }
  
  /// Puts a character back to the input source, so that the next call
  /// to readChar() will return that character.
  void unreadChar(int ch) const { std::ungetc(ch, stdin); }
  
  std::unique_ptr<Expr> parseNumberExpr();
  std::unique_ptr<Expr> parseBoolExpr();
  std::unique_ptr<Expr> parseParenExpr();
  std::unique_ptr<Expr> parseIdentifierExpr();
  std::unique_ptr<Expr> parsePrimaryExpr();
  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<Expr> parseUnaryExpr();
  std::unique_ptr<Expr> parseBinOpRHS(int exprPrecedence, std::unique_ptr<Expr> lhs);
  std::unique_ptr<Expr> parseIfExpr();
  std::unique_ptr<Prototype> parseFnPrototype();
  
  int getTokenPrecedence(int token) const;
  
private:
  int currentToken;
  std::string identifierValue; // Filled in if TokenIdentifier.
  double numberValue; // Filled in if TokenNumber.
  std::unordered_map<int, int> binaryOperatorPrecedence;
  std::unordered_map<std::string, int> idToTokenMap;
};

}

#endif
