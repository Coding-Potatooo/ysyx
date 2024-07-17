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
#include "watchpoint.h"

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = (WP *)malloc(sizeof(WP));
  sprintf(head->expr_str, "%s", "VIRTUAL HEAD of Watchpoints");
  head->next = NULL;
  free_ = (WP *)malloc(sizeof(WP));
  sprintf(free_->expr_str, "%s", "VIRTUAL HEAD of free nodes");
  free_->next = wp_pool;
}

WP *new_wp(char *expr_str)
{
  WP *wp_new = NULL;

  if (free_->next) // return the new node
  {
    WP *tmp = head->next;
    head->next = free_->next;
    free_->next = head->next->next;
    head->next->next = tmp;

    wp_new = head->next;
    bool success = false;
    printf("%s\n", expr_str);
    wp_new->old_val = expr(expr_str, &success);
    assert(success);
    snprintf(wp_new->expr_str, sizeof(wp_new->expr_str), "%s", expr_str);
  }
  else // return NULL
  {
    printf("failed to new a watchpoint: have reached the maximum Watchpoint Number[%d]\n", NR_WP);
  }
  return wp_new;
}

bool delete_wp(int index)
{
  WP *loc;
  for (loc = head; loc->next && loc->next->NO != index; loc = loc->next)
    ;
  printf("watchpoint [%d] deleted\n", loc->next->NO);
  return free_wp(loc);
}

bool free_wp(WP *wp)
{ // wp should be the preceding node. It's fine if wp->next is already in free_ link list.
  bool success = false;
  if (wp->next)
  {
    WP *tmp = free_->next;
    free_->next = wp->next;
    wp->next = free_->next->next;
    free_->next->next = tmp;
    success = true;
  }
  else
  {
    printf("failed to delete a watchpoint: no node found after node%p\n", wp);
  }
  return success;
}

void watchpoints_display()
{
  // printf("Num \t\tType\t\t Disp\t\tEnb\t\tAddress\t\tWhat\n");

  printf("Num \t\tWhat\n");
  for (WP *loc = head; loc->next; loc = loc->next)
  {
    printf("%d\t\t%s\n", loc->next->NO, loc->next->expr_str);
  }
}

bool watchpoints_check()
{
  bool change_flag = false;
  for (WP *loc = head; loc->next; loc = loc->next)
  {
    bool success = false;
    word_t new_val = expr(loc->next->expr_str, &success);
    assert(success);
    if (new_val != loc->next->old_val)
    {
      printf("Watchpoint[%d] {%s} changed from [%u] to [%u]\n", loc->next->NO, loc->next->expr_str, loc->next->old_val, new_val);
      loc->next->old_val = new_val;
      change_flag = true;
      nemu_state.state = NEMU_STOP;
    }
  }

  return change_flag;
}