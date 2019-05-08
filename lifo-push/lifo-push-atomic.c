// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from Maged Michael's pseudocode in WG14 N2369.
//
// This version loads and stores pointer via atomic operations, illustrating
// what the code would look like if the C standard "rejuvenated" pointers
// loaded and stored via atomic operations (including RMW atomics).

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>

typedef char *value_t;

struct node_t {
	value_t val;
	struct node_t *_Atomic next;
};

void set_value(struct node_t *p, value_t v)
{
	p->val = v;
}

void foo(struct node_t *p);

// LIFO list structure
struct node_t *_Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));

	set_value(newnode, v);
	// This store is not a data race, just rejuvenating the pointer
	atomic_store_explicit(&newnode->next, atomic_load(&top), memory_order_relaxed);
	do {
		// newnode->next may have become invalid
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, newnode));
}


void list_pop_all()
{
	struct node_t *p = atomic_exchange(&top, NULL);

	while (p) {
		// This load is not a data race, just rejuvenating the pointer
		struct node_t *next = atomic_load_explicit(&p->next, memory_order_relaxed);

		foo(p);
		free(p);
		p = next;
	}
}

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
