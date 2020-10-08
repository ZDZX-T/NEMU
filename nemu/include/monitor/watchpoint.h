#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	char data[32];
	int val;

} WP;

WP* new_wp();
void free_wp(WP *wp);
void info_w();
bool checkWP();
void delWP(int NO);
#endif
