#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
static WP *wptemp;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].oldValue = 0;
    wp_pool[i].hitNum = 0;
  }
  wp_pool[NR_WP - 1].next = NULL;
    head = (WP*)malloc(sizeof(WP));
    head->NO = 0;
    head->next = NULL;
    head->oldValue = 0;
    head->hitNum = 0;

    free_ = (WP*)malloc(sizeof(WP));
    free_->NO = 0;
    free_->next = wp_pool;
    free_->oldValue = 0;
    free_->hitNum = 0;
}

void new_wp(char *args) { 
    //从free链表中返回一个空闲监视点结构 
    if(free_->next == NULL) { 
        assert(0); 
    } 

    WP* result = free_->next; 
    free_->next = free_->next->next; 
    result->next = NULL;
    strcpy(result->e, args); 
    bool is_success = false; 
    result->oldValue = expr(result->e, &is_success);
    if(is_success == false) { 
        printf("error in new_wp; expression fault!\n"); 
        return;
    } 

    //对head链表进行更新
    wptemp = head; 
    while (wptemp->next != NULL) //找到最后一个结点
    { 
        wptemp = wptemp->next;  
    } 
    wptemp->next = result; 
    
    printf("Success: set watchpoint %d, oldValue = %d\n", result->NO, result->oldValue); 
    return;
}

void free_wp(int num)
{
    wptemp = head;
    WP *remove = head->next;
    WP *temp = free_->next;
    if(remove == NULL) {
        printf("Error: watchpoint %d do not exist!\n",num);
        return;
    }
    if(num > 31 || num < 0) {
        assert(0); 
    }
    while(remove != NULL && remove->NO != num) {
        remove = remove->next;
        wptemp = wptemp->next;
    }
    if(remove == NULL) {
        printf("Error: watchpoint %d do not exist!\n",num);
        return;
    }
    wptemp->next = remove->next;
    remove->oldValue = 0;
    remove->hitNum = 0;
    memset(remove->e,0,sizeof(remove->e));
    wptemp = free_;
    while(temp->next != NULL && temp->NO < remove->NO) {
        temp = temp->next;
        wptemp = wptemp->next;
    }
    wptemp->next = remove;
    remove->next = temp;
    printf("Success: remove watchpoint %d\n",remove->NO);
    return;
}
void print_wp()
{
    wptemp = head->next;
    if(wptemp == NULL) {
        printf("No watchpoint exist!\n");
        return;
    }
    printf("%-12s%-12s%-12s%-12s%-12s\n","watchpoint:","NO.","oldValue","expr","hitTimes"); 
    while(wptemp!=NULL) {
        printf("%-12s%-12d%-12d%-12s%-12d\n", " ",wptemp->NO, wptemp->oldValue,wptemp->e,wptemp->hitNum); 
        wptemp = wptemp->next;
    }
    return;
}

bool watch_wp() { //判断监视点是否触发的辅助函数
    bool is_success; 
    int result; 
    if(head == NULL) { //如果head为空则直接返回
        return true; 
    }  
    wptemp = head->next; //从head开始遍历
    while (wptemp != NULL) 
    { 
        result = expr(wptemp->e, &is_success); 
        if(result != wptemp->oldValue) 
        { 
            wptemp->hitNum += 1; 
            printf("Hardware watchpoint %d:%s\n", wptemp->NO, wptemp->e); 
            printf("Old value:%d\nNew valus:%d\n\n", wptemp->oldValue, result); 
            wptemp->oldValue = result; 
            return false; 
        } 
        wptemp = wptemp -> next; 
    } 
    return true;
}

/* TODO: Implement the functionality of watchpoint */