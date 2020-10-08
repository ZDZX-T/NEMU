#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
	if(free_!=NULL)
	{
		
		WP *aFreeWP=free_;
		free_=free_->next;
		aFreeWP->next=NULL;//init wp
		aFreeWP->val=0;
		aFreeWP->data[0]='\0';

		WP *tail;
		tail=head;
		if(head==NULL)
		{
			head=aFreeWP;
			aFreeWP->NO=1;
			return aFreeWP;
		}
		while(tail->next!=NULL)
			tail=tail->next;
		tail->next=aFreeWP;
		aFreeWP->NO=tail->NO + 1;
		return aFreeWP;
	}
	return NULL;
}

void free_wp(WP *wp){
	WP *left;
	if(head==wp)
	{
		if(head->next==NULL)
			head=NULL;
		else
			head=head->next;
		wp->next=free_;
		free_=wp;
		return;
	}
	left=head;
	while(left->next!=wp)
			left=left->next;
	left->next=wp->next;
	wp->next=free_;
	free_=wp;
}

void info_w(){
	WP *p;
	p=head;
	printf("NO\tinfo\t\tval\n");
	while(p!=NULL)
	{
		printf("%d\t%s\t0x%x(hex) %d(deg)\n",p->NO,p->data,p->val,p->val);
		p=p->next;
	}
}

bool checkWP(){
	WP *p;
	p=head;
	bool suc,flag=0;
	uint32_t newVal;
	while(p!=NULL)
	{
		newVal=expr(p->data,&suc);
		if(p->val != newVal)
		{
			flag=1;
			printf("Watchpoint %d(%s) changed:from %d to %d (deg)\n",p->NO,p->data,p->val,newVal);
			p->val=newVal;
		}
		p=p->next;
	}
	return flag;
}

void delWP(int NO){
	WP *p;
	p=head;
	while(p->NO!=NO && p!=NULL)
		p=p->next;
	if(p==NULL)
	{
		printf("error:watchpoint %d didn't exist\n",NO);
		return;
	}
	free_wp(p);
}