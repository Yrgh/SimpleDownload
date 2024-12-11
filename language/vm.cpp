#ifndef _VM_CPP_
#define _VM_CPP_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SWITCH_CASE(_case, _code) \
case _case: \
{_code} break;

/* Exit codes:
0 - Ok
1 - Memory access bounds check failed
2 - Invalid argument
3 - Memory allocation failed

10 - Invalid instruction
11 - Invalid SPECCALL id
12 - Invalid instruction parameter

20 - Invalid execution state
*/

typedef unsigned char byte;

enum : byte { // If no register is specified, assume left
  OPCODE_CALL,
  OPCODE_RETURN,
  
  OPCODE_LOADSP, // Load from the stack
  OPCODE_LOADFP, // Load from the current stack frame
  OPCODE_LOADIR, // Indirect load from the pointer in the right register
  OPCODE_LOADC,  // Load a constant
  
  OPCODE_SWAP, // Swaps the registers
  
  OPCODE_STORESP, // Stores to the stack
  OPCODE_STOREFP, // Stores to the current stack frame
  OPCODE_STOREIR, // Indirect store to the pointer in the right registers
  
  OPCODE_JMP, // Sets the program counter
  OPCODE_JMPZ, // JMP if last CMP was zero
  OPCODE_JMPN, // JMP if last CMP underflowed
  // Example: x >= y = LOAD y, SWAP, LOAD x, USUB, JMPN after
  
  OPCODE_PUSH, // Pushes a register
  OPCODE_POP,  // Pops a register
  
  OPCODE_CMP, // Unsigned integer subtraction that modifies the zero and 
  // Arithmetc (NEG not for uints)
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_DIV,
  OPCODE_NEG,
  // Float-specific arithmetic
  OPCODE_FFLOOR,
  OPCODE_FCEIL,
  OPCODE_FTRIG, // Like SPECCALL, but for trig functions
  
  OPCODE_AND, // Bitwise AND
  OPCODE_OR,  // Bitwise OR
  OPCODE_XOR, // Bitwise XOR
  OPCODE_NOT, // Reverse bits
  
  OPCODE_SPECCALL, // Call a VM function of given ID

  REG_LEFT  = 0x00,
  REG_RIGHT = 0x08,
  
  TYPE_UNSIGNED = 0x00,
  TYPE_SIGNED   = 0x01,
  TYPE_FLOAT    = 0x02,
  
  TYPE_SIZE_8  = 0x00,
  TYPE_SIZE_16 = 0x01,
  TYPE_SIZE_32 = 0x02,
  TYPE_SIZE_64 = 0x03,
};

#define FROM_SIZE(size) (TYPE_SIZE_##size)

#define UPPER(x) (x >> 4)
#define LOWER(x) (x & 0x0F)
#define MERGE(u, d) ((u << 4) | d)

template<class T> constexpr T max(T a, T b) { return a > b ? a : b; }
template<class T> constexpr T min(T a, T b) { return a < b ? a : b; }

constexpr byte best_type(byte a, byte b) {
  // If (a_up > b_up) return a;
  // If (b_up > a_up) return b;
  // // a_up == b_up
  // return MERGE(a_up, max(a_low, b_low))
  return UPPER(a) > UPPER(b) ? a : (UPPER(b) > UPPER(a) ? b : MERGE(UPPER(a), max(LOWER(a), LOWER(b))));
}

struct VM {
  byte *instructions = nullptr;
  int instructions_size = 0;
  int prog_counter = 0;
  byte registers[8*2] = {};
  byte *stack_base = nullptr;
  byte *stack_ptr;
  byte *frame_ptr;
  uint32_t stack_size = 0;
  
  void push(const void *data, int size) {
    memcpy(stack_ptr, data, size);
    stack_ptr += size;
  }
  
  void pop(void *dest, int size) {
    stack_ptr -= size;
    memcpy(dest, stack_ptr, size);
  }
  
  #define GET_BYTES(num) (instructions + (prog_counter+=num)-num)
  void execute_one() {
    if (prog_counter >= instructions_size) exit(20);
    switch(*GET_BYTES(1)) {
      SWITCH_CASE(OPCODE_LOADC, {
        char size = 1 << (*GET_BYTES(1));
        uint32_t pos = *(uint32_t *) GET_BYTES(4);
        if (instructions_size < pos + size) exit(1);
        memcpy(registers, instructions + pos, size);
      })
      SWITCH_CASE(OPCODE_SWAP, {
        uint64_t left = *(uint64_t *) registers;
        *(uint64_t *) registers = *(uint64_t *) (registers + 8);
        *(uint64_t *) (registers + 8) = left;
      })
      #define APPLY_OP(type, op) \
      do { \
        *(type *) registers op##= *(type *) (registers + 8); \
        break; \
      } while(false) \
      
      #define OP_CASE(name, op) \
      SWITCH_CASE(OPCODE_##name, { \
        switch (*GET_BYTES(1)) { \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(8)): \
            APPLY_OP(uint8_t , op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(16)): \
            APPLY_OP(uint16_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(32)): \
            APPLY_OP(uint32_t, op); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(64)): \
            APPLY_OP(uint64_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(8)): \
            APPLY_OP( int8_t , op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(16)): \
            APPLY_OP( int16_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(32)): \
            APPLY_OP( int32_t, op); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(64)): \
            APPLY_OP( int64_t, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(32)): \
            APPLY_OP(float, op); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(64)): \
            APPLY_OP(double, op); \
            break; \
        } \
      })
      
      OP_CASE(ADD, +)
      OP_CASE(SUB, -)
      OP_CASE(MUL, *)
      OP_CASE(DIV, /)
      
      #undef APPLY_OP
      #define APPLY_OP(type) \
      do { \
        *(type *) registers = - (*(type *) registers); \
        break; \
      } while(false) \
      
      SWITCH_CASE(OPCODE_NEG, { \
        switch (*GET_BYTES(1)) { \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(8)): \
            APPLY_OP(uint8_t); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(16)): \
            APPLY_OP(uint16_t); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(32)): \
            APPLY_OP(uint32_t); \
            break; \
          case MERGE(TYPE_UNSIGNED, FROM_SIZE(64)): \
            APPLY_OP(uint64_t); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(8)): \
            APPLY_OP( int8_t); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(16)): \
            APPLY_OP( int16_t); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(32)): \
            APPLY_OP( int32_t); \
            break; \
          case MERGE(TYPE_SIGNED, FROM_SIZE(64)): \
            APPLY_OP( int64_t); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(32)): \
            APPLY_OP(float); \
            break; \
          case MERGE(TYPE_FLOAT, FROM_SIZE(64)): \
            APPLY_OP(double); \
            break; \
        } \
      })
      
      SWITCH_CASE(OPCODE_RETURN, {
        pop(&prog_counter, 4);
      })
    }
  }
  
  void execute() {
    prog_counter = -10;
    push(&prog_counter, 4);
    prog_counter = 0;
    do {
      execute_one();
    } while(prog_counter >= 0);
  }
  
  void init() {
    stack_base = new byte[16]; // 4 * uint32_t/int32_t/float
    stack_ptr  = stack_base;
    frame_ptr  = stack_base;
  }
  #undef GET_BYTES
  #undef APPLY_OP
  #undef OP_CASE
};

#undef SWITCH_CASE

#endif // _VM_CPP_
