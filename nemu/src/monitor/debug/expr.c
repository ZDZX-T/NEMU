#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,
	
	/* TODO: Add more token types */
	NUMBER,
	HEX,
	REG,
	NEQ,
	AND,
	OR,
	POINTER,
	NEGATIVE,
};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE,0},				// spaces
	{"\\+", '+',11},					// plus
	{"-",'-',11},//	-
	{"\\*",'*',14},//	*
	{"/",'/',14},//	/
	{"==", EQ,8},						// equal
	{"\\b[0-9]+\\b",NUMBER,0},   //number, 123
	{"\\b0[xX][0-9a-fA-F]+\\b",HEX,0},//number, 0xff
	{"\\(",'(',15},// (
	{"\\)",')',15},// )
	{"\\$[a-zA-Z]+",REG,0},//register
	{"!=",NEQ,8},// not equal
	{"!",'!',14},// not
	{"&&",AND,4},// and
	{"\\|\\|",OR,3},// or
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

	for(i = 0; i < NR_REGEX; i ++) {//NR_REGEX: number of rules
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {//NR_REGEX: number of rules
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
					case NOTYPE:break;
					default: 
						tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].priority=rules[i].priority;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len]='\0';
						nr_token++;
						break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parentheses(int p,int q){
	int i,l,r;
	if(tokens[p].type=='(' && tokens[q].type==')')
	{
		l=0;
		r=0;
		for(i=p+1;i<q;i++)
		{
			if(tokens[i].type=='(')
				l++;
			else if(tokens[i].type==')')
				r++;
			if(r>l)
				return false;
		}
		if(l==r)
			return true;
	}
	return false;
}

int dominant_operator(int p,int q){
	int i,op=0;
	int min_priority=17;
	for(i=p;i<=q;i++)
	{
		if(tokens[i].type==NUMBER || tokens[i].type==HEX || tokens[i].type==REG )
			continue;
		if(tokens[i].type=='(')
			while(tokens[i].type!=')')
				i++;
		if(tokens[i].priority<=min_priority)
		{
			op=i;
			min_priority=tokens[op].priority;
		}
	}
	return op;
}

uint32_t eval(int p,int q){
	if(p>q)
	{
		printf("unknown wrong?\n");
		return 0;
	}
	else if(p==q)//should be a "number"
	{
		uint32_t num=0;
		if(tokens[p].type==NUMBER)
		{
			sscanf(tokens[p].str,"%d",&num);
		}
		else if(tokens[p].type==HEX)
		{
			sscanf(tokens[p].str,"%x",&num);
		}
		else if(tokens[p].type==REG)
		{
			int i=0;
			while(tokens[p].str[i]!='\0')//delete '$'
			{
				tokens[p].str[i]=tokens[p].str[i+1];
				i++;
			}
			for(i=0;i<8;i++)
			{
				if(strcmp(tokens[p].str,regsl[i])==0)
					num=reg_l(i);
			}
			if(i>=8 && strcmp(tokens[p].str,"eip")==0)
				num=cpu.eip;
			else
			{
				printf("error: the register should be 32 bits\n");
				return 0;
			}
		}
		return num;
	}
	else if(check_parentheses(p,q)==true)
	{
		return eval(p+1,q-1);
	}
	else
	{
		int op=dominant_operator(p,q);
		if(tokens[op].type==POINTER || tokens[op].type==NEGATIVE ||tokens[op].type=='!')
		{
			uint32_t val=eval(p+1,p+1);
			switch(tokens[op].type)
			{
				case POINTER:return swaddr_read(val,4);
				case NEGATIVE:return -val;
				case '!':return !val;
				default:assert(0);
			}
		}
		uint32_t val1=eval(p,op-1);
		uint32_t val2=eval(op+1,q);
		switch(tokens[op].type)
		{
			case '+':return val1+val2;
			case '-':return val1-val2;
			case '*':return val1*val2;
			case '/':return val1/val2;
			case EQ:return val1==val2;
			case NEQ:return val1!=val2;
			case AND:return val1&&val2;
			case OR:return val1||val2;
			default:assert(0);
		}
	}
	
}

bool helpExplane(int type){//used in expr
	switch (type)
	{
		case NUMBER: return false;
		case HEX:return false;
		case ')':return false;
		case REG:return false;
		default:return true;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	*success = true;
	//explane '*' as pointer
	int i;
	for(i=0;i<nr_token;i++)
	{
		if(tokens[i].type=='*' && (i==0 || helpExplane(tokens[i-1].type)))
		{
			tokens[i].type=POINTER;
			tokens[i].priority=14;
		}
		else if(tokens[i].type=='-' && (i==0 || helpExplane(tokens[i-1].type)))
		{
			tokens[i].type=NEGATIVE;
			tokens[i].priority=14;
		}
	}
	return eval(0,nr_token-1);
}

