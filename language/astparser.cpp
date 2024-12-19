#ifndef _AST_CPP_
#define _AST_CPP_

#include "lexer.cpp"
#include <stdio.h>

static void printIndent(int indent) {
  for (int i = 0; i < indent; ++i) {
    printf("  ");
  }
}

static void printOp(TokenType op) {
  switch (op) {
    case TokenType::PLUS:
      printf("+");
      break;
    case TokenType::MINUS:
      printf("-");
      break;
    case TokenType::SLASH:
      printf("/");
      break;
    case TokenType::STAR:
      printf("*");
      break;
    case TokenType::EX:
      printf("!");
      break;
    case TokenType::EQ:
      printf("=");
      break;
    case TokenType::EQ_EQUAL:
      printf("==");
      break;
    case TokenType::EX_EQUAL:
      printf("!=");
      break;
    case TokenType::GT:
      printf(">");
      break;
    case TokenType::GT_EQUAL:
      printf(">=");
      break;
    case TokenType::LT:
      printf("<");
      break;
    case TokenType::LT_EQUAL:
      printf("<=");
      break;
    case TokenType::CAR:
      printf("^");
      break;
    case TokenType::AMP:
      printf("&");
      break;
    case TokenType::PIP:
      printf("|");
      break;
    case TokenType::DOT:
      printf(".");
      break;
    default:
      printf("??: %d", (int) op);
  }
}

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual void print(int indent) const = 0;
};

struct NumberNode : ASTNode {
  Token tok;
  explicit NumberNode(Token tk) : tok(tk) {}
  
  void print(int indent) const override {
    printIndent(indent);
    printf("%.*s\n", tok.length, tok.start);
  }
};

struct BinaryNode : ASTNode {
  TokenType op;
  ASTNode *left, *right;
  
  explicit BinaryNode(TokenType op, ASTNode *left, ASTNode *right) : 
    left(left), right(right), op(op) {}
  
  ~BinaryNode() {
    if (left)  delete left;
    if (right) delete right;
  }
  
  void print(int indent) const override {
    left->print(indent + 1);
    printIndent(indent);
    printOp(op);
    printf("\n");
    right->print(indent + 1);
  }
};

struct UnaryNode : ASTNode {
  TokenType op;
  ASTNode *expr;
  
  explicit UnaryNode(TokenType op, ASTNode *expr) : 
    expr(expr), op(op) {}
  
  ~UnaryNode() {
    if (expr) delete expr;
  }
  
  void print(int indent) const override {
    printIndent(indent);
    printOp(op);
    printf("\n");
    expr->print(indent + 1);
  }
};

struct IdentifierNode : ASTNode {
  const char *id;
  int length;
  explicit IdentifierNode(const char *name, int len) : id(name), length(len) {}
  
  void print(int indent) const override {
    printIndent(indent);
    printf("%.*s\n", length, id);
  }
};

class Parser {
  Lexer lexer;
  Token previous, current;
  const char *source_code;
  
  void advance() {
    previous = current;
    current  = lexer.getNext();
  }
  
  int getPrec(TokenType type) {
    switch (type) {
      case TokenType::COMMA:
        return 0;
      case TokenType::PIP:
        return 1;
      case TokenType::CAR:
        return 2;
      case TokenType::AMP:
        return 3;
      case TokenType::EQ_EQUAL:
      case TokenType::EX_EQUAL:
        return 4;
      case TokenType::GT:
      case TokenType::LT:
      case TokenType::GT_EQUAL:
      case TokenType::LT_EQUAL:
        return 5;
      case TokenType::PLUS:
      case TokenType::MINUS:
        return 6;
      case TokenType::STAR:
      case TokenType::SLASH:
        return 7;
      case TokenType::DOT:
        return 8;
    }
    return -1;
  }
  
  ASTNode *parsePrimary() {
    switch (current.type) {
      case TokenType::NUMBER: {
        advance();
        return new NumberNode(previous);
      }
      case TokenType::IDENTIFIER: {
        advance();
        return new IdentifierNode(previous.start, previous.length);
      }
      case TokenType::LEFT_ROUND: {
        advance();
        ASTNode *result = parseExpr();
        // Consume ')' afterward
        if (current.type != TokenType::RIGHT_ROUND) printf("Expected ')'\n");
        advance();
        return result;
      }
      // Unary operators
      case TokenType::EX:
      case TokenType::MINUS: {
        advance();
        TokenType op = previous.type;
        return new UnaryNode(op, parsePrimary());
      }
      default:
        printf("Invalid expression!\n");
        return nullptr;
    }
  }
  
  ASTNode *parseBinaryRHS(int min_prec, ASTNode *lhs) {
    while (true) {
      int op_prec = getPrec(current.type);
      if (op_prec < min_prec) break;
      
      Token op = current;
      advance();
      
      ASTNode *rhs = parsePrimary();
      int next_prec = getPrec(current.type);
      if (op_prec < next_prec) {
        rhs = parseBinaryRHS(op_prec + 1, rhs);
      }
      
      lhs = new BinaryNode(op.type, lhs, rhs);
    }
    return lhs;
  }
  
  ASTNode *parseExpr() {
    ASTNode *left = parsePrimary();
    return parseBinaryRHS(0, left);
  }

public:
  ASTNode *top;
  
  void parse(const char *source) {
    source_code = source;
    lexer.init(source);
    
    advance();
    
    top = parseExpr();
  }
};

#endif // _AST_CPP_
