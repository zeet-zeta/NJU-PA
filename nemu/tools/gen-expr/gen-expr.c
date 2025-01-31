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

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

const unsigned int MAX_UNSIGNED_INT = 4294967295;

static inline int choose(int n) {
  return rand() % n;
}

static inline unsigned int gen_num() {
  return rand() % MAX_UNSIGNED_INT;
}

static inline char gen_op() {
  switch (choose(3)) {
    case 0: return '+';
    case 1: return '-';
    case 2: return '*';
  }
  return '+';
}

static void gen_rand_expr_r(int *pos) {
  if (*pos > 500) return;
  switch (choose(3)) {
    case 0:
      unsigned int num = gen_num();
      *pos += sprintf(buf + *pos, "%u", num);
      break;
    case 1:
      buf[*pos] = '(';
      (*pos)++;
      gen_rand_expr_r(pos);
      buf[*pos] = ')';
      (*pos)++;
      break;
    case 2:
      gen_rand_expr_r(pos);
      buf[*pos] = gen_op();
      (*pos)++;
      gen_rand_expr_r(pos);
      break;
  }
}

static void gen_rand_expr() {
  int pos = 0;
  gen_rand_expr_r(&pos);
  buf[pos] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
