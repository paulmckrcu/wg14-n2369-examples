// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from lifo-push.c, adding RCU protection from ABA.
//
//	This protection is stronger (and, within list_pop_all(),
//	more expensive) than strictly required, but it has the useful
//	side-effect of preventing the algorithm from seeing indeterminate
//	pointers.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>

#define _GNU_SOURCE
#define _LGPL_SOURCE
#ifndef DO_QSBR
#define RCU_SIGNAL
#include <urcu.h>
#else /* #ifndef DO_QSBR */
#include <urcu-qsbr.h>
#endif /* #else #ifndef DO_QSBR */

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
	rcu_read_lock();
	newnode->next = atomic_load(&top);
	do {
		// newnode->next may have been removed from the list, but
		// RCU prevents it from being freed.  Thus this pointer
		// remains valid throughout.
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, newnode));
	rcu_read_unlock();
}


void list_pop_all()
{
	struct node_t *p = atomic_exchange(&top, NULL);

	while (p) {
		struct node_t *next = p->next;

		foo(p);
		synchronize_rcu();
		free(p);
		p = next;
	}
}

#include "lifo-stress.h"
