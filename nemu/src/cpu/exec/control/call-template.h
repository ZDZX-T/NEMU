#include "cpu/exec/template-start.h"

#define instr call

make_helper (concat(call_i_,SUFFIX)){
    int length=concat(decode_i_,SUFFIX)(eip+1);
    reg_l(R_ESP)-=DATA_BYTE;
    swaddr_write(reg_l(R_ESP),4,cpu.eip + length);
    print_asm("call %x",cpu.eip + 1 + length + op_src->val);
    cpu.eip+=op_src->val;
    return length+1;
}

make_helper(concat(call_rm_,SUFFIX)){
    int length=concat(decode_rm_,SUFFIX)(eip+1);
    reg_l(R_ESP)-=DATA_BYTE;
    swaddr_write(reg_l(R_ESP),4,cpu.eip + length);
    print_asm("call %x",op_src->val);
    cpu.eip=op_src->val - length -1;
    return length+1;
}

#include "cpu/exec/template-end.h"