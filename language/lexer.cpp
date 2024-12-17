#ifndef _LEXER_CPP_
#define _LEXER_CPP_

#include <string.h>
#include <stdio.h>

enum class TokenType {
  // Error goes first so that null tokens are error tokens! 
  ERROR, EOF_TOKEN,
  
  LEFT_ROUND , RIGHT_ROUND,
  LEFT_SQUARE, RIGHT_SQUARE,
  LEFT_CURLY , RIGHT_CURLY,
  
  COMMA, DOT,
  MINUS, MINUS_EQ,
  PLUS,  PLUS_EQ,
  STAR,  STAR_EQ,
  SLASH, SLASH_EQ,
  
  EX, EX_EQUAL,
  EQ, EQ_EQUAL,
  GT, GT_EQUAL,
  LT, LT_EQUAL,
  
  IDENTIFIER, STRING, NUMBER,
  
  KEY_LET, KEY_IF, KEY_ELSE, KEY_WHILE, KEY_FUNC, KEY_RETURN,
  
  SEMI
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
  
  Token string() {
    start = current; // Exclude the "
    while (peek() != '"' && !atEnd()) {
      if (peek() == '\n')
        return errorToken("Unterminated string at newline");
      else if (peek() == '\\') {
        advance(); // We don't want to let it end the string
        if (atEnd()) break;
      }
      advance();
    }
    
    if (atEnd()) return errorToken("Unterminated string at EOF");
    
    Token tok = makeToken(TokenType::STRING); // Don't include the "
    advance(); // Consume the other "
    return tok;
  }
  
  bool isAlpha(char c) {
    return 
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      c == '_';
  }
  
  bool isDigit(char c) {
    return (c >= '0') && (c <= '9');
  }
  
  Token number() {
    while (isDigit(peek())) advance();
    
    if (peek() == '.')
      while (isDigit(peek())) advance();
    
    return makeToken(TokenType::NUMBER);
  }
  
  TokenType checkKeyword(int from, int length, const char *rest, TokenType type) {
    if (
      current - start == from + length &&
      memcmp(start + from, rest, length) == 0
    ) {
      return type;
    }
    
    return TokenType::IDENTIFIER;
  }
  
  TokenType wordType() {
    switch (start[0]) {
      case 'e': return checkKeyword(1, 3, "lse", TokenType::KEY_ELSE);
      case 'f':
        if (current - start > 1) {
          switch (start[1]) {
            case 'u': return checkKeyword(2, 2, "nc", TokenType::KEY_FUNC);
          }
        }
        break;
      case 'i': return checkKeyword(1, 1, "f", TokenType::KEY_IF);
      case 'l': return checkKeyword(1, 2, "et", TokenType::KEY_LET);
      case 'r': return checkKeyword(1, 5, "eturn", TokenType::KEY_RETURN);
      case 'w': return checkKeyword(1, 4, "hile", TokenType::KEY_WHILE);
    }
    
    return TokenType::IDENTIFIER;
  }
  
  Token word() {
    char c = peek();
    while (isAlpha(c) || isDigit(c)) {
      advance();
      c = peek();
    }
    
    return makeToken(wordType());
  }
public:
  const char *start, *current;
  int line;
  
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
    if (isDigit(c)) return number();
    if (isAlpha(c)) return word();
    switch (c) {
      case '(': return makeToken(TokenType::LEFT_ROUND);
      case ')': return makeToken(TokenType::RIGHT_ROUND);
      case '[': return makeToken(TokenType::LEFT_SQUARE);
      case ']': return makeToken(TokenType::RIGHT_SQUARE);
      case '{': return makeToken(TokenType::LEFT_CURLY);
      case '}': return makeToken(TokenType::RIGHT_CURLY);
      case ',': return makeToken(TokenType::COMMA);
      case '.': return makeToken(TokenType::DOT);
      case ';': return makeToken(TokenType::SEMI);
      case '!': return makeToken(
        match('=') ? TokenType::EX_EQUAL : TokenType::EX);
      case '=': return makeToken(
        match('=') ? TokenType::EQ_EQUAL : TokenType::EQ);
      case '>': return makeToken(
        match('=') ? TokenType::GT_EQUAL : TokenType::GT);
      case '<': return makeToken(
        match('=') ? TokenType::LT_EQUAL : TokenType::LT);
      case '-': return makeToken(
        match('=') ? TokenType::MINUS_EQ : TokenType::MINUS);
      case '+': return makeToken(
        match('=') ? TokenType::PLUS_EQ  : TokenType::PLUS);
      case '/': return makeToken(
        match('=') ? TokenType::SLASH_EQ : TokenType::SLASH);
      case '*': return makeToken(
        match('=') ? TokenType::STAR_EQ  : TokenType::STAR);
      case '"':
        return string();
      
    }
    
    return errorToken("No matching type!\n");
  }
};

#endif // _LEXER_CPP_
