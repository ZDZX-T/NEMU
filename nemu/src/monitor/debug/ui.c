#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

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

static int cmd_si(char *args){
	int N=0;
	if(args==NULL)
	{
		N=1;
	}
	else
	{
		sscanf(args,"%d",&N);
	//	printf("%d\n",N);    //test N
	}
	
	if(N>=1)
	{
		if(N>=10)
		{
			printf("N > 9, program will run %d steps but will not print any information\n",N);
		}	
		cpu_exec(N);
	}
	else
	{
		printf("error:N < 1\n");
	}
	return 0;
}

static int cmd_info(char *args){
	if(args[0]=='r')
	{
		int i;
		for (i=0;i<8;i++)
			printf("%s : 0x%08x\n",regsl[i],reg_l(i));
		printf("eip : 0x%08x\n",cpu.eip);
	}
	else if(args[0]=='w')
	{
		info_w();
	}
	else
	{
		printf("Unknow command.\n");
	}
	return 0;
}

static int cmd_x(char *args){
	int N;
	int i,j;
	bool flag;
	swaddr_t targetMemory;
	char *cmd=strtok(args," ");
	sscanf(cmd,"%d",&N);
	args=cmd+strlen(cmd)+1;
	targetMemory=expr(args,&flag);
	if(flag==false)
	{
		printf("bad expression\n");
		return 0;
	}
	for(i=0;i<N;i++)
	{
		if(i%2 == 0)
		{
			if(i)//print enter if it is not the first line
				printf("\n");
			printf("0x%08x : \t",targetMemory);
		}
		for(j=0;j<4;j++)
		{
			printf("%02x ",swaddr_read(targetMemory,1));
			targetMemory+=1;
		}
		printf("\t");
	}
	printf("\n");
	return 0;
}

static int cmd_p(char *args){
	uint32_t result;
	bool flag;
	result=expr(args,&flag);
	if(flag)
		printf("result:\t0x%x(hex)\t%d(deg)\n",result,result);
	else
		printf("error: bad expression\n");
	return 0;
}

static int cmd_w(char *args){
	WP *newWP=new_wp();
	if(newWP==NULL)
	{
		printf("error:watchpoint pool is empty\n");
		return 0;
	}
	bool flag;
	strcpy(newWP->data,args);
	newWP->val=expr(newWP->data,&flag);
	if(flag)
	{
		printf("Set watchpoint--  NO:%d  %s  val:%x(hex) %d(deg)\n",newWP->NO,newWP->data,newWP->val,newWP->val);
	}
	else
	{
		printf("error:bad watchpoint information\n");
		free_wp(newWP);
	}
	return 0;
}

static int cmd_d(char *args){
	int NO;
	sscanf(args,"%d",&NO);
	delWP(NO);
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

	/* TODO: Add more commands */
	{ "si","Pause after N steps, N is 1 when not set",cmd_si},
	{"info","Print the status of the program, r for register, w for watchpoint",cmd_info},
	{"x","Scan memory",cmd_x},
	{"p","Expresison evaluation",cmd_p},
	{"w","Set watchpoint",cmd_w},
	{"d","Delete watchpoint",cmd_d},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
