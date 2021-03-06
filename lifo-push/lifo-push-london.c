// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from lifo-push.c based on suggestions by Jens Gustedt.

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

void foo(struct node_t *p);

// LIFO list structure
struct node_t* _Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));

	set_value(newnode, v);
	newnode->next = NULL;
	do {
		// newnode->next may have become indeterminate, which might
		// result in list_pop_all() loading an indeterminate pointer
		// from ->next whose address points to a reallocated object.
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, newnode));
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

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
