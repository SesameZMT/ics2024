#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
    int oldValue;
    char e[32];
    int hitNum;
} WP;

void new_wp(char *arg); //新建监视点
void free_wp(int num);  //删除监视点
void print_wp();        //打印监视点
bool watch_wp();        //监视点值变化

#endif