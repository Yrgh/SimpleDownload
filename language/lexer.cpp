#ifndef _LEXER_CPP_
#define _LEXER_CPP_

#include <string.h>

enum class TokenType {
  LEFT_ROUND , RIGHT_ROUND,
  LEFT_SQUARE, RIGHT_SQUARE,
  LEFT_CURLY , RIGHT_CURLY,
  
  COMMA, DOT,
  MINUS, PLUS, STAR, SLASH,
  
  EX_MARK, EX_EQUAL,
  EQ_MARK, EQ_EQUAL,
  GT_MARK, GT_EQUAL,
  LT_MARK, LT_EQUAL,
  
  IDENTIFIER, STRING, NUMBER,
  
  KEY_LET,
  
  SEMI, 
  
  ERROR, EOF_TOKEN,
};

struct Token {
  TokenType type;
  const char *start;
  int length;
  int line;
};

class Lexer {
  bool atEnd() { return *current == '\0'; }
  
  Token makeToken(TokenType type) {
    Token token;
    token.line = line;
    token.start = start;
    token.length = (int) (current - start);
    token.type = type;
    return token;
  }
  
  Token errorToken(const char *message) {
    Token token;
    token.start = message;
    token.length = strlen(message);
    token.type = TokenType::EOF_TOKEN;
    token.line = line;
    return token;
  }
  
  char advance() {
    current++;
    return current[-1];
  }
  
  bool match(char expected) {
    if (atEnd()) return false;
    if (*current != expected) return false;
    current++;
    return true;
  }
  
  char peek() {
    return *current;
  }
  
  char peekNext() {
    return current[1];
  }
  
  void ignoreSpace() {
    for (;;) {
      char c = peek();
      switch (c) {
        case ' ':
        case '\r':
        case '\t':
          advance();
          break;
        case '\n':
          line++;
          advance();
          break;
        case '/':
          // Consume it
          advance();
          // Single-line comment
          if (peek() == '/') {
            advance();
            while (peek() != '\n' && !atEnd()) advance();
          } else if (peek() == '*') {
            // Consume the *
            advance();
            while (true) {
              if (atEnd()) break;
              if (peek() == '*' && peekNext() == '/') {
                advance(); // Consume *
                advance(); // Consume /
                break;
              }
              advance(); // Continue
            }
          }
          break;
        default:
          return;
      }
    }
  }
  
  const char *start, *current;
  int line;
public:
  void init(const char *source) {
    start = source;
    current = source;
    line = 0;
  }
  
  Token getNext() {
    ignoreSpace();
    start = current;
    
    if (atEnd()) return makeToken(TokenType::EOF_TOKEN);
    
    char c = advance();
    switch (c) {
      case '(': return makeToken(TokenType::LEFT_ROUND);
      case ')': return makeToken(TokenType::RIGHT_ROUND);
      case '[': return makeToken(TokenType::LEFT_SQUARE);
      case ']': return makeToken(TokenType::RIGHT_SQUARE);
      case '{': return makeToken(TokenType::LEFT_CURLY);
      case '}': return makeToken(TokenType::RIGHT_CURLY);
      case ',': return makeToken(TokenType::COMMA);
      case '.': return makeToken(TokenType::DOT);
      case '-': return makeToken(TokenType::MINUS);
      case '+': return makeToken(TokenType::PLUS);
      case '*': return makeToken(TokenType::STAR);
      case '/': return makeToken(TokenType::SLASH);
      case ';': return makeToken(TokenType::SEMI);
      case '!': return makeToken(
        match('=') ? TokenType::EX_EQUAL : TokenType::EX_MARK);
      case '=': return makeToken(
        match('=') ? TokenType::EQ_EQUAL : TokenType::EQ_MARK);
      case '>': return makeToken(
        match('=') ? TokenType::GT_EQUAL : TokenType::GT_MARK);
      case '<': return makeToken(
        match('=') ? TokenType::LT_EQUAL : TokenType::LT_MARK);
      
    }
    
    return errorToken("No matching type!\n");
  }
};

#endif // _LEXER_CPP_
