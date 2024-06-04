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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#define BUF_SIZE 65536
// this should be enough
static char buf[BUF_SIZE] = {};
static char code_buf[BUF_SIZE + 128] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";
int buf_index = 0;
int max_iterate_times = 30;
int iterate_times = 0;
uint32_t choose(uint32_t n)
{
  return rand() % n;
}

static void gen(char c)
{
  buf[buf_index++] = c;
}

static void gen_rand_op()
{
  char op[4] = {'+', '-', '*', '/'};
  int index = choose(4);
  gen(op[index]);
}

static void gen_num()
{
  int max = 1000;
  uint32_t num = rand() % max;
  sprintf(buf + buf_index, "%d", num);
  int length = num ? (int)(log10(abs(num)) + 1) : 1;
  buf_index += length;
  buf[buf_index++] = 'u'; //treat as unsigned int
}

static void gen_rand_expr()
{
  iterate_times += 1;
  switch (choose(10))
  {
  case 0:
    gen_num();
    break;
  case 1:
    if (iterate_times < max_iterate_times)
    {
      gen('(');
      gen_rand_expr();
      gen(')');
    }
    else
    {
      gen_num();
    }

    break;
  case 2: // Generate space randomly
    if (iterate_times < max_iterate_times)
    {
      gen(' ');
      gen_rand_expr();
    }
    else
    {
      gen_num();
    }
    break;

  case 3: // Generate space randomly
    if (iterate_times < max_iterate_times)
    {
      gen_rand_expr();
      gen(' ');
    }
    else
    {
      gen_num();
    }
    break;

  default:
    if (iterate_times < max_iterate_times)
    {
      gen_rand_expr();
      gen_rand_op();
      gen_rand_expr();
    }
    else
    {
      gen_num();
    }
  }

  buf[buf_index] = '\0';
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++)
  {
    buf_index = 0;
    iterate_times = 0;
    gen_rand_expr();
    sprintf(code_buf, code_format, buf);
    if (buf_index >= BUF_SIZE)
    {
      printf("Expression is too long that the buf is overflow!\n");
      assert(0);
    }
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr"); // filter out exprs that divide 0
    if (ret != 0)
      continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
