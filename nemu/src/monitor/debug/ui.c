#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
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
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
    uint64_t N = 0;
    if(args == NULL) { // 若无输入值则N默认为1
        N = 1;
    }
    else {
        int temp = sscanf(args,"%llu",&N);
        if(temp <= 0) {
            printf("args error in cmd_si\n");
            return 0;
        }
    }
    cpu_exec(N);
    return 0;
}

static int cmd_info(char *args) {
    char s;
    if(args == NULL) {
        printf("args error in cmd_info (miss args)\n");
        return 0;
    }
    int temp = sscanf(args, "%c", &s);
    if(temp <= 0) {
        //解析失败
        printf("args error in cmd_info\n");
        return 0;
    }
    if(s == 'w') {
        print_wp();
        return 0;
    }

    if(s == 'r') {
        //打印寄存器
        //32bit
        for(int i = 0; i < 8; i++) {
        printf("%s  0x%x\n", regsl[i], reg_l(i));
        }
        printf("eip  0x%x\n", cpu.eip);
        //16bit
        for(int i = 0; i < 8; i++) {
        printf("%s  0x%x\n", regsw[i], reg_w(i));
        }
        //8bit
        for(int i = 0; i < 8; i++)
        {
        printf("%s  0x%x\n", regsb[i], reg_b(i));
        }
        printf("eflags:CF=%d,ZF=%d,SF=%d,IF=%d,OF=%d\n", cpu.eflags.CF, cpu.eflags.ZF, cpu.eflags.SF, cpu.eflags.IF, cpu.eflags.OF);
        printf("CR0=0x%x, CR3=0x%x\n", cpu.CR0, cpu.CR3);
        return 0;
    }

    //如果产生错误
    printf("args error in cmd_info\n");
    return 0;
}

static int cmd_x(char *args) {
    int len = 0;
    char *t = (char *)malloc(30*sizeof(char));
    vaddr_t addr;
    int temp = sscanf(args,"%d %s",&len,t);
    if(temp <= 0) {
        printf("args error in cmd_si\n");
        return 0;
    }
    bool success=false;
    addr=expr(t,&success);
    printf("Memory:\n");
    for(int i = 0;i < len;i++) {
        if(i % 4 == 0) { 
            printf("0x%x:", addr);
            uint32_t val=vaddr_read(addr,4);
            uint8_t *by=(uint8_t *)&val;
            printf("0x");
            for(int j=3;j>=0;j--) {
                printf("%02x",by[j]);
            }
            printf("\n");
            addr+=4;
        } 
    }
    return 0;
}

static int cmd_p(char *args) {
  //表达式求值
  bool is_success;
  int temp = expr(args, &is_success);
  if(is_success == false) {
    printf("error in expr()\n");
  }
  else {
    printf("the value of expr is:%d\n", temp);
  }
  return 0;
}

static int cmd_w(char *args) { //监视点的申请
    char *s1 = (char*)malloc(6*sizeof(char));
    char *s2 = (char*)malloc(20*sizeof(char));
    int temp = sscanf(args,"%s %s",s1,s2);
    if(temp <= 0) {
        printf("args error in cmd_w\n");
        return 0;
    }
    if(strcmp(s1,"set") == 0) {
        new_wp(s2);
        return 0;
    }
    else if(strcmp(s1,"remove") == 0) {
        int n = 0;
        temp = sscanf(s2,"%d",&n);
        free_wp(n);
        return 0;
    }
    return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "args:[N]; exectue [N] instructions step by step", cmd_si}, //让程序单步执行 N 条指令后暂停执行, 当N没有给出时, 默认为1
  { "info", "args:r/w;print information about register or watch point ", cmd_info}, //打印寄存器状态
  { "x", "x [N] [EXPR];sacn the memory", cmd_x }, //内存扫描
  { "p", "expr", cmd_p}, //表达式
  { "w", "set:set the watchpoint\n    remove:remove the watchpoint\n", cmd_w}, //添加监视点
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

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
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
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

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
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