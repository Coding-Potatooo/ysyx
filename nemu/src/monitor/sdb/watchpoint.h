#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#define NR_WP 32

typedef struct watchpoint
{
    int NO;
    struct watchpoint *next;

    char expr_str[1024];
    word_t old_val;

    bool enable;
} WP;

void init_wp_pool();

void watchpoints_display();
bool watchpoints_check();

WP *new_wp(char *expr);
bool delete_wp(int index);
bool free_wp(WP *wp);

#endif