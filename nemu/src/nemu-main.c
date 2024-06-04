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

#include <common.h>
#include "./monitor/sdb/sdb.h"
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
void test_expr();

int main(int argc, char *argv[])
{
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

#define TEST_EXPR 1
#ifdef TEST_EXPR
  test_expr();
#endif

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}

void test_expr()
// #include "./monitor/sdb/sdb.h"
{
  printf("I am testing!\n");
  char *nemu_home = getenv("NEMU_HOME");
  char input_path[1024];
  sprintf(input_path, "%s%s", nemu_home, "/tools/gen-expr/build/input");
  // printf("%s\n", input_path);
  FILE *file = fopen(input_path, "r");
  assert(file);

  char input_str[65536] = {0};
  while (fgets(input_str, 65536, file))
  {
    // printf("%s\n", input_str);
    // int golden_result = 0;

    // remove \n at the end of the line, other wise \n will not be recongnized by make_tokens()
    int input_str_len = strlen(input_str);
    if (input_str_len > 0 && input_str[input_str_len - 1] == '\n')
    {
      input_str[input_str_len - 1] = 0;
    }
    char *delimiter_pos = strchr(input_str, ' ');
    assert(delimiter_pos);
    *delimiter_pos = 0;
    char *golden_result_str = input_str;
    char *expr_str = delimiter_pos + 1;

    bool *success = false;
    word_t eval_result = expr(expr_str, success);
    word_t golden_result = atoi(golden_result_str);
    if (golden_result != eval_result)
    {
      printf("BUG FOUND!!!\n");
      // printf("\t[golden_result_str]:%s\n\t[expr_str]:%s\n\teval_result:%d\n", golden_result_str, expr_str, eval_result);
      // Log("\t[golden_result_str]:%s\n\t[expr_str]:%s\n\teval_result:%d\n", golden_result_str, expr_str, eval_result);
    }
  }
}