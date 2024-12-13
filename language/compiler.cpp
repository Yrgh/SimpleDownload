#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include "vm.cpp"
#include "lexer.cpp"
#include <stdio.h>
#include <vector>

class Parser {
  Lexer lexer;
  
  struct Replacement {
    int dest;
    void *bytes;
    int length;
  };
  
  std::vector<Replacement> replace_data;
  std::vector<byte> out_buf;
  
  template<class T> Replacement createReplacement(int location, T item) {
    Replacement r;
    r.bytes = malloc(sizeof(T));
    *(T *) r.bytes = item;
    r.length = sizeof(T);
    r.dest = location;
    return r;
  }
  
  void emitByte(byte b) {
    out_buf.push_back(b);
  }
  
  void emitPair(byte data, byte b) {
    out_buf.push_back(data);
    out_buf.push_back(b);
  }
  
  void emitNulls(int num) {
    for (int i = 0; i < num; ++i) {
      out_buf.push_back(0);
    }
  }
  
  byte number() {
    emitByte(OPCODE_LOADC);
    advance(); // We (should) know its type. We'll still be able to see it in 'previous'
    if (strchr(previous.start, '.') == NULL) {
      // It's an integer
      uint32_t num = strtoull(previous.start, NULL, 10);
      if (num < 1ull << 8) {
        emitNulls(1);
        createReplacement<uint8_t>(out_buf.size(), num);
        return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
      }
      if (num < 1ull << 16) {
        emitNulls(2);
        createReplacement<uint16_t>(out_buf.size(), num);
        return MERGE(TYPE_UNSIGNED, FROM_SIZE(16));
      }
      if (num < 1ull << 32) {
        emitNulls(4);
        createReplacement<uint32_t>(out_buf.size(), num);
        return MERGE(TYPE_UNSIGNED, FROM_SIZE(32));
      }
      emitNulls(8);
      createReplacement<uint64_t>(out_buf.size(), num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(64));
    } // It's a float, but which one?
    double num = strtod(previous.start, NULL);
    if (
      abs(num) < 3.4028235677973366e+38 &&
      abs(num) > 1.175494351e-38
    ) {
      // Congratulations! It's a float! I think...
      emitNulls(4);
      // It'll convert
      createReplacement<float>(out_buf.size(), num);
      return MERGE(TYPE_FLOAT, FROM_SIZE(32));
    }
    emitNulls(8);
    createReplacement<double>(out_buf.size(), num);
    return MERGE(TYPE_FLOAT, FROM_SIZE(64));
  }
  
  Token previous, current;
  bool hadError;
  bool panicMode;
  
  void advance() {
    previous = current;
    
    while (true) {
      current = lexer.getNext();
      if (current.type != TokenType::ERROR) break;
      
      printf("Token Error: %s\n", current.start);
    }
  }
  
  void consume(TokenType type, const char *message) {
    if (current.type == type) {
      advance();
      return;
    }
    
    printf("Parse Error: %s\n", message);
  }
  
  void convert(byte from, byte to) {
    emitByte(OPCODE_CONV);
    emitPair(from , to);
  }
  
  byte expression() {
    
  }
  
  byte unary() {
    switch (current.type) {
      case TokenType::MINUS: {
        advance(); // Consume the operator. We already know its type
        
        byte type = unary();
        // You can't negate an unsigned, so make it signed
        if (UPPER(type) == TYPE_UNSIGNED) {
          byte newt = MERGE(TYPE_SIGNED, LOWER(type));
          convert(type, newt);
          type = newt;
        }
        emitPair(OPCODE_NEG, type);
        return type;
      }
    }
    // If all else fails...
    return parsePrimary();
  }
  
  byte grouping() {
    byte type = expression();
    consume(TokenType::RIGHT_ROUND, "Expected ')'");
    return type;
  }
  
  byte binary(int prec = 0) {
    byte left = unary();
    tempStore(TYPE_TO_LEFT(left)); // Hold on until later, I think
    
    while (true) {
      TokenType op = current.type;
      advance(); // "Gobble gobble gobble!" - Turkey
      int op_prec = getPrec(op);
      if (op_prec < prec) break;
      
      byte right = binary(op_prec + 1);
      
    }
    
  }
  
  int getPrec(TokenType op) {
    switch (op) {
      case TokenType::EQ_MARK:
        return 1;
      case TokenType::PLUS:
      case TokenType::MINUS:
        return 2;
      case TokenType::STAR:
      case TokenType::SLASH:
        return 3;
      case TokenType::EX_MARK:
        return 4;
      default:
        return 0;
    }
  }
  
  byte identifierPointer() {
    return TYPE_NONE;
  }
  
  byte identifier() {
    byte type = identifierPointer();
    emitPair(OPCODE_LOAD, LOWER(type));
    return type;
  }
  
  void tempStore(byte reg) {
    emitPair(OPCODE_PUSH, reg);
  }
  
  void tempLoad(byte reg) {
    emitByte(OPCODE_SWAP);
    emitPair(OPCODE_POP, reg);
  }
  
  byte parseAssign() {
    byte right = expression();
    tempStore(TYPE_TO_LEFT(right));
    byte left = identifierPointer();
    tempLoad(TYPE_TO_LEFT(right));
    
    if (right != left) {
      convert(right, left);
    }
    
    return left;
  }
  
  byte parsePrimary() {
    switch (current.type) {
      case TokenType::NUMBER:
        return number();
      case TokenType::IDENTIFIER:
        return identifier();
      case TokenType::LEFT_ROUND:
        advance();
        return grouping();
    }
    printf("Non-conforming expression\n");
  }
  
  void fixResult() {
    while (!replace_data.empty()) {
      Replacement &data = replace_data.back();
      int32_t *dest = (int32_t *) (out_buf.data() + data.dest);
      *dest = out_buf.size();
      out_buf.insert(out_buf.end(), (byte *) data.bytes, (byte *) data.bytes + data.length);
      free(data.bytes);
      replace_data.pop_back();
    }
  }
public:
  bool compile(const char *source) {
    out_buf.clear();
    hadError = false;
    panicMode = false;
    lexer.init(source);
    
    expression();
    consume(TokenType::EOF_TOKEN, "Expect end of expression\n");
    
    if (hadError) return false;
    fixResult();
    return true;
  }
};

#endif // _COMPILER_CPP_
