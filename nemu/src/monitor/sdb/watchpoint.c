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

#include "sdb.h"

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].watch_expr[0] = '\0';
    wp_pool[i].value = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char* watch_expr) {
  if (free_ == NULL) {
    Log("no free space for watchpoint");
    return NULL;
  }
  if (strlen(watch_expr) > NR_EXPR) {
    Log("too long expr");
    return NULL;
  }
  //取一个节点从free_到head
  WP *old_head = head;
  head = free_;
  free_ = free_->next;
  head->next = old_head;
  strncpy(head->watch_expr, watch_expr, NR_EXPR);
  bool success;
  word_t value = expr(watch_expr, &success);
  if (success) {
    head->value = value;
  } else {
    Log("expr compute err");
    head->value = 0;
  }
  return head;
}

void free_up(WP* wp) {
  //取一个节点从head到free_
  if (wp == NULL) {
    Log("shouldn't be null");
    return;
  }
  memset(wp->watch_expr, 0, sizeof(wp->watch_expr));
  wp->value = 0;
  if (wp == head) {
    head = wp->next;
  } else {
    WP *prev = head;
    for (; prev->next != wp; prev = prev->next);
    prev->next = wp->next;
  }
  wp->next = free_;
  free_ = wp;
}

void scan_all(bool* has_change) {
  if (head == NULL) {
    return;
  }
  for (WP* cur = head; cur != NULL; cur = cur->next) {
    bool success;
    word_t new_value = expr(cur->watch_expr, &success);
    if (new_value != cur->value) {
      printf("watchpoint %d: %s change form %u to %u\n", cur->NO, cur->watch_expr, cur->value, new_value);
      cur->value = new_value;
      *has_change = true;
    }
  }
}

void print_all() {
  if (head == NULL) {
    Log("no watchpoint");
    return;
  }
  for (WP* cur = head; cur != NULL; cur = cur->next) {
    printf("watchpoint %d: %s = %u\n", cur->NO, cur->watch_expr, cur->value);
  }
}

void delete_by_NO(int num) {
  if (head == NULL) {
    return;
  }
  for (WP* cur = head; cur != NULL; cur = cur->next) {
    if (cur->NO == num) {
      free_up(cur);
      printf("delete watchpoint %d\n", num);
      return;
    }
  }
  Log("can't find");
}
