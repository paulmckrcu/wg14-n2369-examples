// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from lifo-push.c based on suggestions by Jens Gustedt
//	(substitute CAS for load) and Martin Sebor (casts to uintptr_t).

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

typedef char *value_t;

struct node_t {
	value_t val;
	uintptr_t next;
};

void set_value(struct node_t *p, value_t v)
{
	p->val = v;
}

void foo(struct node_t *p);

// LIFO list structure
uintptr_t _Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));

	set_value(newnode, v);
	newnode->next = (uintptr_t)NULL;
	do {
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, (uintptr_t)newnode));
}


void list_pop_all()
{
	struct node_t *p = (struct node_t *)atomic_exchange(&top, (uintptr_t)NULL);

	while (p) {
		struct node_t *next = (struct node_t *)p->next;

		foo(p);
		free(p);
		p = next;
	}
}

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
