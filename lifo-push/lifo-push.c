#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>

typedef char *value_t;

struct node_t {
	value_t val;
	struct node_t *next;
};

void set_value(struct node_t *p, value_t v)
{
	p->val = v;
}

void foo(struct node_t *p)
{
	(*p->val)++;
}

// LIFO list structure
struct node_t* _Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));

	set_value(newnode, v);
	newnode->next = atomic_load(&top);
	while (1) {
		// newnode->next may have become invalid
		if (atomic_compare_exchange_weak(&top, &newnode->next, newnode))
			break;
	}
}


void list_pop_all()
{
	struct node_t *p = atomic_exchange(&top, NULL);

	while (p) {
		struct node_t *next = p->next;

		foo(p);
		free(p);
		p = next;
	}
}

#include "fifo-stress.h"
