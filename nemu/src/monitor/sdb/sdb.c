/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sdb/sdb.h>
#include <sdb/watchpoint.h>

#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1); // -1 is interpreted as unsigned int, the max unsigned int.
  return 0;
}

static int cmd_q(char *args)
{
  // set_nemu_state(NEMU_QUIT, 0, 0);
  nemu_state.state = NEMU_QUIT; // in this way, halt_pc and halt_ret is perseved 
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
  int times = 1; // default
  if (args)
  {
    times = atoi(args);
  }
  cpu_exec(times);
  return 0;
}

static int cmd_info(char *args)
{
  if (args == NULL)
  {
    printf("Please provide subcmd: [r|w], r for register, w for watchpoint\n");
  }
  else if (strcmp(args, "r") == 0)
  {
    isa_reg_display();
  }
  else if (strcmp(args, "w") == 0)
  {
    watchpoints_display();
  }
  else
  {
    printf("Invalid Argument:%s\n", args);
  }

  return 0;
}

static int cmd_x(char *args)
{
  char *len_str = strtok(args, " ");
  int len = atoi(len_str);
  char *addr_str = strtok(NULL, " ");
  char **endptr = NULL;
  vaddr_t addr = strtoull(addr_str, endptr, 16);
  printf("addr\t\t\t value\n");
  for (int i = 0; i < len; i++)
  {
    printf("[%x]\t\t%x\n", addr, vaddr_read(addr, 4));
    addr += 4; // N*4Bytes
  }

  return 0;
}

static int cmd_p(char *args)
{
  bool success = false;
  word_t res = expr(args, &success);
  assert(success);
  printf("%u\n", res);
  return 0;
}

static int cmd_px(char *args)
{
  bool success = false;
  word_t res = expr(args, &success);
  assert(success);
  printf("0x%x\n", res);
  return 0;
}

static int cmd_w(char *args)
{
  new_wp(args);
  return 0;
}

// delete watchpoints
static int cmd_d(char *args)
{
  int n = atoi(args);
  delete_wp(n);
  return 0;
}

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Let the program step through N instructions and then pause execution. If N is not given, the default value is 1", cmd_si},
    {"info", "Print information specified by subcommand", cmd_info},
    {"x", "Print memory: evaluate expression, say V, then print the next N bytes starting from V", cmd_x},
    {"p", "Evaluate expression in decimal", cmd_p},
    {"px", "Evaluate expression in heximal", cmd_px},
    {"w", "Set wacth point", cmd_w},
    {"d", "Delete break point", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given, e.g.
      (nemu) help
      show all description 
    */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    /* (nemu) help <arg>
      show description of <arg>
    */
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        // function call.
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
