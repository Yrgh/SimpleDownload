#ifndef _COMPILER_CPP_
#define _COMPILER_CPP_

#include "vm.cpp"
#include "astparser.cpp"
#include <vector>

class Compiler {
  struct AddData {
    int where; // Where to place it
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
    emitNulls(sizeof(T));
    add_data.push_back(ad);
  }
  
  void incorporateAddData() {
    while (!add_data.empty()) {
      AddData &ad = add_data.back();
      memcpy(out_buf.data() + ad.where, ad.what, ad.length);
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
  
  void eval(ASTNode *node_base) {
    
  }
public:
  void compile(const char *source) {
    Parser parser;
    parser.parse(source);
    eval(parser.top);
    emitByte(OPCODE_RETURN);
    incorporateAddData();
  }
  
  const byte *getResultData() const { return out_buf.data(); }
  int getResultSize() const { return out_buf.size(); }
};

#endif // _COMPILER_CPP_
