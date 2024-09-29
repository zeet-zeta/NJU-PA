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
#include <errno.h>
#include <limits.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256,
  TK_EQ,
  TK_LB,//left bracket
  TK_RB,
  TK_PLUS,
  TK_MINUS,
  TK_MULTIPLE,
  TK_DIVIDE,
  TK_DEC,
  TK_HEX,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_DEREF,
  TK_NEGATIVE,
  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"0x[0-9a-fA-F]+", TK_HEX},
  {"0|([1-9][0-9]*)", TK_DEC},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", TK_MINUS},
  {"\\*", TK_MULTIPLE},
  {"/", TK_DIVIDE},
  {"\\(", TK_LB},
  {"\\)", TK_RB},
  {"&&", TK_AND},
  {"!=", TK_NEQ},
  {"\\$0|\\$\\$0|\\$[0-9a-z]+", TK_REG},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};//存储编译后的正则表达式

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}//编译

typedef struct token {
  int type;
  char str[100];
} Token;

static Token tokens[5000] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    if (nr_token == ARRLEN(tokens)) {
      Log("tokens overflow");
      return false;
    }
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case TK_DEC:
          case TK_HEX:
          case TK_REG:
            if (substr_len > 99) {
              Log("too long");
              return false;
            } else {
              tokens[nr_token].type = rules[i].token_type;
              for (int j = 0; j < substr_len; j++) {
                tokens[nr_token].str[j] = *(substr_start + j);
              }
              tokens[nr_token].str[substr_len] = '\0';
              nr_token++;
            }
            break;
          case TK_MULTIPLE:
          case TK_MINUS:
            if (nr_token == 0 ||
              (tokens[nr_token - 1].type != TK_DEC &&
              tokens[nr_token - 1].type != TK_HEX &&
              tokens[nr_token - 1].type != TK_REG &&
              tokens[nr_token - 1].type != TK_RB)) {
                if (rules[i].token_type == TK_MULTIPLE) {
                  tokens[nr_token].type = TK_DEREF;
                } else {
                  tokens[nr_token].type = TK_NEGATIVE;
                }
            } else {
              tokens[nr_token].type = rules[i].token_type;
            }
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;
        }
        break; //按顺序匹配到某个正则，退出搜索所有正则
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type != TK_LB || tokens[q].type != TK_RB) {
    return false;
  }

  int counter = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LB) {
      counter++;
    } else if (tokens[i].type == TK_RB) {
      counter--;
      if (counter < 0) {
        return false;
      } else if (counter == 0 && i != q) {
        return false;
      }
    }
  }
  return counter == 0;
} //检查字符串是否被一对括号包围

word_t eval(int p, int q) {
  if (p > q) {
    return 0; //处理一元运算符的val1
  }
  if (p == q) {
    switch (tokens[p].type) {
      case TK_DEC:
      case TK_HEX:
        word_t number;
        char *endptr;
        errno = 0;
        unsigned long temp = strtoul(tokens[p].str, &endptr, tokens[p].type == TK_DEC ? 10 : 16);
        if (errno == ERANGE) {
          Log("exceed the ul range");
        } else if (endptr == tokens[p].str) {
          Log("invalid");
        } else if (temp > __UINT32_MAX__) {
          Log("exceed the word_t range");
        } else {
          number = (word_t) temp;
          return number;
        }
        return (word_t) 0;
        break;
      case TK_REG:
        bool success;
        word_t t = isa_reg_str2val(tokens[p].str + 1, &success);
        if (success) {
          return t;
        } else {
          Log("TK_REG failed");
          return (word_t) 0;
        }
        break;
      default:
        Log("why reach here");
        break;
    }
  } else if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1); //去掉头尾括号
  } else if (p < q) { //最麻烦
    int bracket_counter = 0;
    int priority = 100;
    int op_index = -1;
    for (int i = p; i <= q; i++) {
      int cur = tokens[i].type;
      if (cur == TK_LB) {
        bracket_counter++;
        continue;
      } else if (cur == TK_RB) {
        bracket_counter--;
        if (bracket_counter < 0) {
          Log("invalid bracket");
          return 0;
        }
        continue;
      }

      if (bracket_counter > 0) {
        continue;//还在括号里面
      }

      switch (cur) {
        case TK_DEC:
        case TK_HEX:
        case TK_REG:
          continue;
        case TK_AND:
          if (priority >= 0) {
            priority = 0;
            op_index = i;
          }
          break;
        case TK_EQ:
        case TK_NEQ:
          if (priority >= 1) {
            priority = 1;
            op_index = i;
          }
          break;
        case TK_PLUS:
        case TK_MINUS:
          if (priority >= 2) {
            priority = 2;
            op_index = i;
          }
          break;
        case TK_MULTIPLE:
        case TK_DIVIDE:
          if (priority >= 3) {
            priority = 3;
            op_index = i;
          }
          break;
        case TK_NEGATIVE:
        case TK_DEREF:
          if (priority > 4) {
            priority = 4;
            op_index = i;
          }
          break;
        default:
          Log("why reach here");
      }
    } //找主运算符

    word_t val1 = eval(p, op_index - 1);
    word_t val2 = eval(op_index + 1, q);
    switch (tokens[op_index].type) {
      case TK_PLUS:
        return val1 + val2;
      case TK_MINUS:
        return val1 - val2;
      case TK_MULTIPLE:
        return val1 * val2;
      case TK_DIVIDE:
        return val1 / val2;
      case TK_NEGATIVE:
        word_t neg = -1;
        return neg * val2;
      case TK_DEREF:
        return vaddr_read(val2, 4);
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      case TK_AND:
        return val1 && val2;
      default:
        Log("why reach here");
    }
  }
  Log("why reach here");
  return 0;
}
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  return eval(0, nr_token - 1);
}
