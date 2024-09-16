#include <cpu/decode.h>
#include <generated/autoconf.h>
#include <debug.h>
#include <trace.h>

// ring buf is an queue.
#ifdef CONFIG_IRINGBUF_SIZE
char iringbuf[CONFIG_IRINGBUF_SIZE][128];
int irb_h = 0, irb_t = 0; // h for head, t for tail.

bool irb_full()
{
    return ((irb_t + 1) % CONFIG_IRINGBUF_SIZE == irb_h);
}

void irb_add(char *e)
{
    // add at tail.
    if (irb_full())
    {
        irb_h = (irb_h + 1) % CONFIG_IRINGBUF_SIZE; // evict one from head.
    }
    strcpy(iringbuf[irb_t], e);
    irb_t = (irb_t + 1) % CONFIG_IRINGBUF_SIZE;
}

void dump_iringbuf()
{
    for (int i = irb_h; i != irb_t; i = (i + 1) % CONFIG_IRINGBUF_SIZE)
    {
        ilog_write("%s \n", iringbuf[i]);
    }
}
#endif

void store_inst2logbuf(Decode *s) // store instruction log information to s->logbuf
{
#ifdef CONFIG_ITRACE
    char *p = s->logbuf;
    p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc); // 0x80000000:
    int ilen = s->snpc - s->pc;                               // ilen means for instruction length
    // to print every byte of inst. from high address to low address so that the output is human read friendly...(note that inst stored in memeory in little endian fashion.)
    int i;
    uint8_t *inst = (uint8_t *)&s->isa.inst.val;
    for (i = ilen - 1; i >= 0; i--)
    {
        p += snprintf(p, 4, " %02x", inst[i]);
    }

    int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
    int space_len = ilen_max - ilen;
    if (space_len < 0)
        space_len = 0;
    space_len = space_len * 3 + 1;
    memset(p, ' ', space_len);
    p += space_len;

#ifndef CONFIG_ISA_loongarch32r
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
                MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
    p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

extern func_info func_table[10000];
extern uint32_t func_table_cnt;
int dnpc2func_idx(word_t dnpc)
{
    for (int i = 0; i < func_table_cnt; i++)
    {
        if (func_table[i].begin_addr == dnpc)
        {
            return i;
        }
    }
    return -1;
}

func_info call_stack[10000];
uint32_t call_stack_top = 0;

void ftrace_call(word_t pc, word_t dnpc)
{
#ifdef CONFIG_FTRACE
    int func_idx = dnpc2func_idx(dnpc);
    if (func_idx >= 0) // this is a call instuction!
    {
        func_info callee = func_table[func_idx];
        call_stack[call_stack_top++] = callee;
        flog_write("PC@0x%08x: ", pc);
        for (int d = 0; d < call_stack_top - 1; d++)
        {
            flog_write("\t");
        }
        flog_write("call [%s@0x%08x]\n", callee.name, callee.begin_addr);
    }
#endif
}

void ftrace_ret(word_t pc, word_t dnpc)
{
#ifdef CONFIG_FTRACE
    Assert(call_stack_top > 0, "Wrong: RET before CALL");
    func_info env = call_stack[--call_stack_top];

    flog_write("PC@0x%08x: ", pc);
    for (int d = 0; d < call_stack_top; d++)
    {
        flog_write("\t");
    }
    flog_write("ret [%s]\n", env.name);
#endif
}
