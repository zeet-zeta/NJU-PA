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
#include "sdb.h"
#include <memory/vaddr.h>
#include <utils.h>

static int is_batch_mode = false; //非交互

void init_regex();
void init_wp_pool();

extern void read_reg_from_file(const char*);
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
} //继续运行


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
} //退出

static int cmd_help(char *args); //帮助

static int cmd_p(char *args) {
  bool success;
  word_t tmp = expr(args, &success);
  if (success) {
    printf("%u\n", tmp);
  } else {
    printf("err\n");
  }
  return 0;
  
  //以下为随机测试代码
  // read_reg_from_file("/home/zeet/ics2024/reg");
  // bool success;
  // FILE *file = fopen("/home/zeet/ics2024/input", "r");
  // char line[10000];
  // while (fgets(line, sizeof(line), file)) {
  //   // char *token = strtok(line, " ");
  //   // unsigned int expected_result = atoi(token);

  //   // char *expression = strtok(NULL, " \n");
  //   char* space_pos = strchr(line, ' ');
  //   *space_pos = '\0';
  //   char* token = space_pos;
  //   unsigned int expected_result = atoi(line);
  //   token = strchr(space_pos + 1, '\n');
  //   *token = '\0';
  //   char* expression = space_pos + 1;
  //   unsigned int actual_result = expr(expression, &success);
  //   if (actual_result == expected_result) {
  //     printf("Test passed ");
  //   } else {
  //     printf("Test failed: expected %u, got %u for expression: %s\n", expected_result, actual_result, expression);
  //   }
  // }
  // fclose(file);
  // return 0;
}

static int cmd_w(char* args) {
  new_wp(args);
  return 0;
}

static int cmd_d(char* args) {
  delete_by_NO(atoi(args));
  return 0;
}

static int cmd_info(char* args) {
  if (*args == 'w') {
    print_all();
  } else if (*args == 'r') {
    isa_reg_display();
  } else {
    Log("no such command");
  }
  return 0;
}

static int cmd_x(char* args) {
  int n = atoi(strtok(args, " "));
  bool success;
  word_t expr_value = expr(strtok(NULL, " "), &success);
  if (success) {
    for (int i = 0; i < n; i++) {
      printf("0x%.8x : 0x%.8x\n",expr_value + i * 4, vaddr_read(expr_value + i * 4, 4));
    }
  } else {
    Log("invalid expr");
  }
  return 0;
}

static int cmd_si(char *args) {
  int n;
  if (args == NULL) {
    n = 1;
  } else {
    n = atoi(args);
  }
  cpu_exec(n);
  return 0;
}

static int cmd_load_reg(char *args) {
  read_reg_from_file("/home/zeet/ics2024/reg");
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"p", "Expression", cmd_p}, 
  {"w", "set a watchpoint", cmd_w},
  {"d", "delete a watchpoint", cmd_d},
  {"info", "print all watchpoints", cmd_info},
  {"x", "scan the memory", cmd_x},
  {"si", "step", cmd_si},
  {"load_reg", "rt", cmd_load_reg},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
} //帮助

void sdb_set_batch_mode() {
  is_batch_mode = true;
} //设置非交互

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }
//感觉跟AM相关
#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}

