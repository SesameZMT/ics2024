#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, 
  TK_EQ,
  TK_DEC,
  TK_HEX,
  TK_REG,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_NEGATIVE,
  TK_DEREF,

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},         // equal
  {"0x[0-9A-Fa-f][0-9A-Fa-f]*", TK_HEX}, // 十六进制
  {"0|[1-9][0-9]*", TK_DEC}, // 十进制
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh)", TK_REG}, // 寄存器
  {"!=", TK_NEQ}, // 不等
  {"&&", TK_AND}, // 与
  {"\\|\\|", TK_OR}, // 或
  {"!", '!'}, // 非   
  {"-", '-'}, // 减
  {"\\*", '*'}, // 乘
  {"\\/", '/'}, // 除
  {"\\(", '('}, // 左括号
  {"\\)", ')'}, // 右括号
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

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
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
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

                if(rules[i].token_type == TK_NOTYPE) { //跳过空格
                    break; 
                } 
                else { 
                    tokens[nr_token].type = rules[i].token_type; 
                    switch (rules[i].token_type) { 
                        case TK_DEC: // 十进制
                            strncpy(tokens[nr_token].str, substr_start, substr_len); 
                            *(tokens[nr_token].str + substr_len) = '\0'; 
                            break; 
                        case TK_HEX: // 十六进制
                            strncpy(tokens[nr_token].str, substr_start + 2, substr_len - 2); // 跳过开头的0x 
                            *(tokens[nr_token].str + substr_len - 2) = '\0'; 
                            break; 
                        case TK_REG:  // 寄存器
                            strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1); // 跳过开头的$ 
                            *(tokens[nr_token].str + substr_len - 1) = '\0'; 
                    } 
                    printf("Success record : nr_token = %d, dtype = %d, str = %s\n", nr_token, tokens[nr_token].type, tokens[nr_token].str); 
                    nr_token += 1; //记录成功后，nr_token加1
                    break; 
                } 
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}

// 判断括号的匹配 
bool check_parentheses(int p, int q) {
    if(p >= q) {
        // 右括号个数少于左括号
        printf("error: p >= q in check_parntheses\n");
        return false;
    }
    if(tokens[p].type != '(' || tokens[q].type != ')'){
        // 括号不匹配
        return false;
    }
    int cnt = 0; // 记录当前未匹配的左括号的数目
    for(int curr = p + 1; curr < q; curr++) {
        if(tokens[curr].type == '(') { // 遇到左括号，数目加1
            cnt++;
        }
        if(tokens[curr].type == ')') { // 遇到右括号，数目减1
            if(cnt != 0) {
                cnt--;
            }
            else {
                // 左右括号不匹配
                return false;
            }
        }
    }
    if(cnt == 0) {
        return true;
    }
    else {
        return false;
    }
} 

int findDominantOp(int p, int q) {
    /* 找主运算符
     * 也就是整个表达式中运算优先级最低的运算符
     * 找到之后分别计算两边式子的值
     * 最后进行递归
     */
    int level=0;
    int pos[5]={-1, -1, -1, -1, -1};
    for(int i = p; i < q; i++){
        if(level == 0) {
            if(tokens[i].type == TK_AND || tokens[i].type == TK_OR) { 
                // 优先级最低的运算符
                pos[0] = i;
            }
            if(tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) {
                pos[1] = i;
            }
            if(tokens[i].type == '+' || tokens[i].type == '-') {
                pos[2] = i;
            }
            if(tokens[i].type == '*' || tokens[i].type == '/') {
                pos[3] = i;
            }
            if(tokens[i].type == TK_NEGATIVE || tokens[i].type == TK_DEREF || tokens[i].type == '!') { 
                //  优先级最高的运算符
                pos[4] = i;
            }
        }
        if(tokens[i].type=='(') {
            level++;
        }
        if(tokens[i].type==')') {
            level--;
        }
    }
    for(int i = 0; i < 5; i++) {
        if(pos[i] != -1) {
            return pos[i];
        }
    }
    printf("error in findDominantOp\n");
    printf("[p=%d, q=%d]\n",p,q);
    assert(0);
}

uint32_t eval(int p, int q) {
    if(p>q) {
        printf("error:p>q in eval, p = %d, q = %d\n", p, q);
        assert(0);
    }
    else if(p==q) {
        /* Single token.
         * For now this token should be a number
         * Return the value of the number.
         */
          int num;
        switch (tokens[p].type){
            case TK_DEC: //十进制
                sscanf(tokens[p].str, "%d", &num);
                return num;
            case TK_HEX: //16进制数
                sscanf(tokens[p].str, "%x", &num);
                return num;
            case TK_REG: //寄存器
                for(int i = 0; i < 8; i++) {
                    if(strcmp(tokens[p].str, regsl[i]) == 0) {
                        return reg_l(i);
                    }
                    if(strcmp(tokens[p].str, regsw[i]) == 0) {
                        return reg_w(i);
                    }
                    if(strcmp(tokens[p].str, regsb[i]) == 0) {
                        return reg_b(i);
                    }
                }
                if(strcmp(tokens[p].str, "eip") == 0) {
                    return cpu.eip;
                }
                else {
                    printf("error in TK_REG in eval()\n");
                    assert(0);
                } 
        }
    }
    else if(check_parentheses(p,q)== true) {
        /* The expression is surrounded by a matched pair of parentheses.
         * If that is the case, just throw away the parentheses.
         */
        return eval(p+1,q-1);
    }
    else {
        int op = findDominantOp(p, q);//找主运算符
        vaddr_t addr; //TK_DEREF的地址
        int result;
        // 单目运算符
        switch (tokens[op].type) {
            case TK_NEGATIVE: //负号
                printf("Operator= -.\n");
                printf("Value=%d.\n",result);
                return -eval(p + 1, q);
            case TK_DEREF: //指针求值
                addr = eval(p + 1, q);
                result = vaddr_read(addr, 4);
                printf("adddr=%u(0x%x)---->value=%d(0x%08x)\n", addr, addr, result, result);
                return result;
            case '!': 
                result = eval(p + 1, q);
                printf("Operator= !.\n");
                if(result != 0) {
                    printf("Value=0.\n");
                    return 0;
                }
                else {
                    printf("Value=1.\n");
                    return 1;
                }
        }
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);
        switch(tokens[op].type) {
            case '+':
                printf("Operator= +.\n");
                printf("Value=%d.\n",val1+val2);
                return val1 + val2;
            case '-': 
                printf("Operator= -.\n");
                printf("Value=%d.\n",val1-val2);
                return val1 - val2;
            case '/':
                if(val2==0){
                    printf("Error: The val2 can't be 0.\n");
                    assert(0);
                }
                printf("Operator= /.\n");
                printf("Value=%d.\n",val1/val2);
                return val1 / val2;
            case '*':
                printf("Operator= *.\n");
                printf("Value=%d.\n",val1*val2);
                return val1 * val2;
            case TK_EQ:
                printf("Operator= ==.\n");
                printf("Value=%d.\n",val1==val2);
                return val1 == val2;
            case TK_NEQ: 
                printf("Operator= !=.\n");
                printf("Value=%d.\n",val1!=val2);
                return val1 != val2;
            case TK_AND: 
                printf("Operator= &&.\n");
                printf("Value=%d.\n",val1&&val2);
                return val1 && val2;
            case TK_OR: 
                printf("Operator= ||.\n");
                printf("Value=%d.\n",val1||val2);
                return val1 || val2;
            default:
                printf("Error: Invalid operator.\n");
                assert(0);
        }
    }
    return 1;
}

uint32_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    /* TODO: Insert codes to evaluate the expression. */
    //TODO();
    if(tokens[0].type == '-') { //处理
        tokens[0].type = TK_NEGATIVE;
    }
    if(tokens[0].type == '*') { //处理
        tokens[0].type = TK_DEREF; 
    }
    for(int i = 1; i < nr_token; i++) {
        if(tokens[i].type == '-') { 
        // 如果前一个不是数字或者右括号，那么就是负号
        if(tokens[i - 1].type != TK_DEC && tokens[i - 1].type != ')') {
            tokens[i].type = TK_NEGATIVE;
        }
        }
        if(tokens[i].type == '*') {
        // 如果前一个不是数字或者右括号，那么就是指针求值
        if(tokens[i - 1].type != TK_DEC && tokens[i - 1].type != ')') {
            tokens[i].type = TK_DEREF;
        }
        }
    }
    *success = true;
    return eval(0, nr_token - 1);
}