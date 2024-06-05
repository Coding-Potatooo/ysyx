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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

#include "../../../include/memory/vaddr.h"
enum
{
  /* TODO: Add more token types */
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NE,
  TK_GE,
  TK_GT,
  TK_LE,
  TK_LT,

  TK_DEC,
  TK_HEX,

  TK_OR,
  TK_AND,
  TK_NOT,

  TK_LP,
  TK_RP,

  TK_REG,

  TK_DEREF, // dereference

  TK_REV, // reverse

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    /* Priority decreasing from top to bottom.
     */
    {" +", TK_NOTYPE}, // spaces

    {"\\+", '+'}, // plus
    {"\\-", '-'}, // minus
    {"\\*", '*'}, // multiple
    {"\\/", '/'}, // divide

    {"==", TK_EQ}, // equal
    {"!=", TK_NE}, // not equal
    {">=", TK_GE}, // greater or equal
    {"<=", TK_LE}, // less or equal
    {"<", TK_LT},  // greater than
    {">", TK_GT},  // less than

    {"!", TK_NOT}, // logic not

    {"\\(", TK_LP}, // left parenthesis
    {"\\)", TK_RP}, // right parenthesis

    // hex must be before dec!
    {"0[xX][0-9a-fA-F]+", TK_HEX}, // heximal numbers
    {"[0-9]+u?", TK_DEC},          // decimal numbers, u for unsigned int.

    {"\\$[a-z]{0,2}[0-9]{0,2}", TK_REG} // registers

};

#define NR_REGEX ARRLEN(rules) // NR for Number of Rules?

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;
/*
__attribute__((used)) 是 GCC 和 Clang 编译器的扩展语法，用于告诉编译器确保一个变量或函数即使在没有显式引用的情况下也保留在最终的可执行文件中。

通常情况下，如果一个变量或函数没有被使用（即没有被引用），编译器会将其优化掉，以减小最终可执行文件的大小。然而，在某些情况下，我们需要确保某些变量或函数即使没有明显的引用也被保留下来，这时就可以使用 __attribute__((used))。
*/

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  // use nr_token to index static tokens[]
  // every time make_token is called, nr_toekn is initialized to 0, so that tokens[] is fresh.
  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Token token;
        switch (rules[i].token_type)
        {
        case TK_NOTYPE:
          break;

        case TK_REG:
          bool success = false;
          token.type = TK_DEC;
          char regname[4] = {0};
          memcpy(regname, substr_start, substr_len);
          word_t reg_val = isa_reg_str2val(regname, &success);
          if (!success)
          {
            assert(0);
          }
          sprintf(token.str, "%u", reg_val);
          tokens[nr_token++] = token;
          break;

        case TK_HEX:
          // convert heximal to decimal
          char hex_str[16 + 1] = {0};
          char dec_str[20 + 1] = {0};
          memcpy(hex_str, substr_start, substr_len + 2); //+2 to omit the 0x or 0X
          word_t val = 0;
          sscanf(hex_str, "%x", &val);
          sprintf(dec_str, "%d", val);
          token.type = TK_DEC;
          memcpy(token.str, dec_str, 20 + 1);
          tokens[nr_token++] = token;
          break;

        default:
          token.type = rules[i].token_type;
          memcpy(token.str, substr_start, substr_len);
          token.str[substr_len] = 0; // str end with 0
          tokens[nr_token++] = token;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parenthesis(int p, int q)
/*
 if tokens[p] is LP, tokens[q] is RP and *they are a pair!*, return true;
*/
{
  if (tokens[p].type != TK_LP || tokens[q].type != TK_RP)
  {
    return false;
  }
  int stack_idx = 0; // stack_idx == cnt of elements in stack
  while (!(p > q))
  {
    if (tokens[p].type == TK_LP)
    {
      stack_idx++; // push a LP
    }
    else if (tokens[p].type == TK_RP)
    {
      if (stack_idx > 0)
      {
        --stack_idx;                 // pop a LP
        if (stack_idx == 0 && p < q) // the first LP doesnt match the last RP, they are not a pair
        {
          return false;
        }
      }
      else // more RightParenthesis than LeftParenthesis
      {
        return false;
      }
    }
    p++;
  }

  return !(stack_idx > 0); // stack_idx>0 =>more LeftParenthesis than RightParenthesis; parenthesis match only when stack_idx = 0 when p>q
}

word_t eval(int p, int q)
/*Recursive evaluation
 */
{
  if (p > q)
  { /*TODO: handle bad expression
     */
    TODO();
    return -1;
  }
  else if (p == q)
  {
    /*have to guarantee that tokens[p] is a number...
     */
    return atoi(tokens[p].str);
  }
  else if (check_parenthesis(p, q) == true)
  {
    /*Throw the parenthesis
     */
    return eval(p + 1, q - 1);
  }
  else // Given the else if check_parenthesis clause, there is no parenthesis at p,q (there may be parentheses inside...)
  {

    enum priority // must assign as default: 0,1,2... to facilitate "for" iteration.
    {
      /*
      https://blog.csdn.net/changexhao/article/details/82556761
      the value of priority samller, the less prior
      */
      PRI_OR,      // or
      PRI_AND,     // and
      PRI_COMPARE, // ==, !=
      PRI_ADD,     // add, minus
      PRI_MUL,     // multiply, divide
      PRI_SINGLE,  // dereference, reverse, not
    };
    enum priority pri = PRI_ADD; //
    int in_parenthesis = 0;
    bool found_flag = false; // if the maini_op is found, break both for loops(this found_flag is for the out layer)
    int i = p;
    for (pri = 0; pri <= PRI_SINGLE; pri++)
    {
      for (i = q; i >= p; i--)
      {
        if (tokens[i].type == TK_RP)
        {
          in_parenthesis += 1;
        }
        else if (tokens[i].type == TK_LP)
        {
          in_parenthesis -= 1;
        }

        else if (!in_parenthesis && pri == PRI_OR && (tokens[i].type == TK_OR))
        {
          found_flag = true;
          break;
        }

        else if (!in_parenthesis && pri == PRI_AND && (tokens[i].type == TK_AND))
        {
          found_flag = true;
          break;
        }

        else if (!in_parenthesis && pri == PRI_COMPARE &&
                 (tokens[i].type == TK_EQ || tokens[i].type == TK_NE || tokens[i].type == TK_GE ||
                  tokens[i].type == TK_LE || tokens[i].type == TK_LT || tokens[i].type == TK_GT))
        {
          found_flag = true;
          break;
        }

        else if (!in_parenthesis && pri == PRI_ADD && (tokens[i].type == '+' || tokens[i].type == '-'))
        {
          found_flag = true;
          break;
        }

        else if (!in_parenthesis && pri == PRI_MUL && (tokens[i].type == '*' || tokens[i].type == '/'))
        {
          found_flag = true;
          break;
        }
        else if (!in_parenthesis && pri == PRI_SINGLE && (tokens[i].type == TK_DEREF || tokens[i].type == TK_REV || tokens[i].type == TK_NOT))
        {
          found_flag = true;
          break;
        }
      }
      if (found_flag)
      {
        break;
      }
    }

    Token main_op = tokens[i];
    word_t result = 0, res1 = 0, res2 = 0;
    switch (main_op.type)
    {
    case TK_DEREF:
      vaddr_t vaddr = atoi(tokens[i + 1].str);
      result = vaddr_read(vaddr, 1);
      break;
    case TK_REV:
      result = -eval(i + 1, q);
      break;
    case TK_NOT:
      result = eval(i + 1, q) ? 1 : 0;

    case TK_OR:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 || res2) ? 1 : 0;
      break;
    case TK_AND:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 && res2) ? 1 : 0;
      break;
    case TK_NE:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 != res2) ? 1 : 0;
      break;
    case TK_EQ:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 == res2) ? 1 : 0;
      break;
    case TK_GT:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 > res2) ? 1 : 0;
      break;
    case TK_LT:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 < res2) ? 1 : 0;
      break;
    case TK_LE:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 <= res2) ? 1 : 0;
      break;
    case TK_GE:
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = (res1 >= res2) ? 1 : 0;
      break;
    case '*':
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = res1 * res2;
      // printf("%d=%d * %d\n", result, res1, res2);
      break;
    case '/':
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      if (res2 == 0)
      { // divide by 0
        assert(0);
      }
      result = res1 / res2;
      // printf("%d=%d / %d\n", result, res1, res2);
      break;
    case '+':
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = res1 + res2;
      // printf("%d=%d + %d\n", result, res1, res2);
      break;
    case '-':
      res1 = eval(p, i - 1);
      res2 = eval(i + 1, q);
      result = res1 - res2;
      // printf("%d=%d - %d\n", result, res1, res2);
      break;
    default:
      assert(0);
    }
    return result;
  }
}

word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }
  /*Logic to deal with dereference.
    Set * appeared in deference as TK_DEREF;
  */
  for (int i = 0; i < nr_token; i++)
  {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_RP)))
    {
      tokens[i].type = TK_DEREF;
    }
    if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_DEC && tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_RP)))
    {
      tokens[i].type = TK_REV;
    }
  }

  word_t val = eval(0, nr_token - 1);
  return val;
}
