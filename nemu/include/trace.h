#include <cpu/decode.h>

// store instruction log information to s->logbuf
void store_inst2logbuf(Decode *s);

void irb_add(char *e);

void dump_iringbuf();

// ftrace
typedef struct func_info_
{
    char name[100];
    word_t begin_addr, end_addr;
}func_info;

void ftrace_call(word_t pc, word_t dnpc);

void ftrace_ret(word_t pc, word_t dnpc);


