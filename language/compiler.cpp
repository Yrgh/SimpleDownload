#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include "vm.cpp"
#include "astparser.cpp"
#include <vector>
#include <string>

static bool strcontains(const char *big, int off, int len, char item) {
  const char *ptr = strchr(big + off, item);
  if (ptr == NULL) return false;
  return ptr < big + off + len;
}

class Compiler {
  struct AddData {
    int32_t where; // Where to place it
    byte *what;
    int length;
  };
  
  template<class T> void addData(const T &d) {
    AddData ad;
    ad.where = out_buf.size();
    T *buf = new T[1];
    buf[0] = d;
    ad.what = (byte *) buf;
    ad.length = sizeof(T);
    emitNulls(4);
    add_data.push_back(ad);
  }
  
  void incorporateAddData() {
    while (!add_data.empty()) {
      AddData &ad = add_data.back();
      int32_t pos = out_buf.size();
      emitNulls(ad.length);
      memcpy(out_buf.data() + pos, ad.what, ad.length);
      memcpy(out_buf.data() + ad.where, &pos, 4);
      delete[] ad.what;
      add_data.pop_back();
    }
  }
  
  std::vector<AddData> add_data;
  std::vector<byte> out_buf;
  
  void emitByte(byte b) {
    out_buf.push_back(b);
  }
  
  void emitPair(byte b0, byte b1) {
    out_buf.push_back(b0);
    out_buf.push_back(b1);
  }
  
  void emitNulls(int num) {
    for (int i = 0; i < num; ++i) {
      out_buf.push_back(0);
    }
  }
  
  void convert(byte from, byte to) {
    if (from == to) return;
    emitByte(OPCODE_CONV);
    emitPair(from , to);
  }
  
  void tempStore(byte reg) {
    emitPair(OPCODE_PUSH, LOWER(reg));
  }
  
  void tempLoad(byte reg) {
    emitByte(OPCODE_SWAP);
    emitPair(OPCODE_POP, LOWER(reg));
  }
  
  byte processNum(Token tok) {
    emitByte(OPCODE_LOADC);
    if (strcontains(tok.start, 0, tok.length, '.')) {// It's a float, but which one?
      double num = strtod(tok.start, NULL);
      if (
        abs(num) < 3.4028235677973366e+38 &&
        abs(num) > 1.175494351e-38
      ) {
        // Congratulations! It's a float! I think...
        // It'll convert
        emitByte(FROM_SIZE(32));
        addData<float>(num);
        return MERGE(TYPE_FLOAT, FROM_SIZE(32));
      }
      emitByte(FROM_SIZE(64));
      addData<double>(num);
      return MERGE(TYPE_FLOAT, FROM_SIZE(64));
    } // It's an integer
    uint32_t num = strtoull(tok.start, NULL, 10);
    if (num < 1ull << 8) {
      emitByte(FROM_SIZE(8));
      addData<uint8_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(8));
    }
    if (num < 1ull << 16) {
      emitByte(FROM_SIZE(16));
      addData<uint16_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(16));
    }
    if (num < 1ull << 32) {
      emitByte(FROM_SIZE(32));
      addData<uint32_t>(num);
      return MERGE(TYPE_UNSIGNED, FROM_SIZE(32));
    }
    emitByte(FROM_SIZE(64));
    addData<uint64_t>(num);
    return MERGE(TYPE_UNSIGNED, FROM_SIZE(64));
  }
  
  byte processUnary(byte _type_in, TokenType op) {
    byte type = _type_in; // Maybe the compiler doesn't like it?
    switch (op) {
      case TokenType::MINUS:
        if (UPPER(type) == TYPE_UNSIGNED) {
          byte newt = MERGE(TYPE_SIGNED, LOWER(newt));
          convert(type, newt);
          type = newt;
        }
        
        emitPair(OPCODE_NEG, type);
        
        return type;
      case TokenType::EX:
        emitPair(OPCODE_NOT, LOWER(type));
        return type;
    }
    
    printf("Invalid unary operator!\n");
    return TYPE_NONE;
  }
  
  byte processBinary(byte _type_in, TokenType op) {
    byte type = _type_in; // Same as above;
    switch (op) {
      case TokenType::PLUS:
        emitPair(OPCODE_ADD, type);
        return type;
      case TokenType::MINUS:
        emitPair(OPCODE_SUB, type);
        return type;
      case TokenType::STAR:
        emitPair(OPCODE_MUL, type);
        return type;
      case TokenType::SLASH:
        emitPair(OPCODE_DIV, type);
        return type;
    }
    return type;
  }
  
#define CONDITION(type, name) \
if (const type *name = dynamic_cast<const type *>(node_base))

  byte evalExpr(const ASTNode *node_base) {
    if (node_base == nullptr) {
      printf("Null node encountered!\n");
      return TYPE_NONE;
    }
    
    CONDITION(NumberNode, num) {
      return processNum(num->tok);
    }
    
    CONDITION(IdentifierNode, id) {
      std::string name = std::string(id->id, id->length);
      return TYPE_NONE;
    }
    
    CONDITION(UnaryNode, unary) {
      byte type = evalExpr(unary->expr);
      return processUnary(type, unary->op);
    }
    
    CONDITION(BinaryNode, binary) {
      byte left = evalExpr(binary->left);
      tempStore(left);
      byte right = evalExpr(binary->right);
      byte best = best_type(left, right);
      convert(right, best);
      tempLoad(left);
      convert(left, best);
      return processBinary(best, binary->op);
    }
    
    printf("Invalid expression!\n");
    return TYPE_NONE;
  }

#undef CONDITION
public:
  void compile(const char *source) {
    Parser parser;
    parser.parse(source);
    parser.top->print(0);
    evalExpr(parser.top);
    emitByte(OPCODE_RETURN);
    incorporateAddData();
  }
  
  const byte *getResultData() const { return out_buf.data(); }
  int getResultSize() const { return out_buf.size(); }
};

#endif // _COMPILER_CPP_
