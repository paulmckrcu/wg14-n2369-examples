// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from lifo-push.c based on suggestions by Jens Gustedt
//	(substitute CAS for load) and Martin Sebor (casts to uintptr_t).
//
// Additional Changes: Copyright Martin Uecker, 2019
//
// This is a modified version with macro wrappers for type safety.
//
// The code should be well-defined with the integer casts interpreted
// according to the rules of the provenance-aware memory object
// model proposed for C2X in N2362.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

typedef char *value_t;

struct node_ptr_t {

	uintptr_t i;
};

struct node_t {

	value_t val;
	struct node_ptr_t next;
};

#define TYPE_CHECK(T, x) (0?(T){0}:(x))
#define NODE_INT(x)	((struct node_ptr_t){ (uintptr_t)TYPE_CHECK(struct node_t *, x) })
#define NODE_PTR(x)	((struct node_t *)(TYPE_CHECK(struct node_ptr_t, x).i))


void set_value(struct node_t *p, value_t v)
{
	p->val = v;
}

void foo(struct node_t *p);

int list_empty(struct node_ptr_t p)
{
	return (NULL == NODE_PTR(p));
}
#define list_empty(p) list_empty(p)

// LIFO list structure
struct node_ptr_t _Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));

	set_value(newnode, v);
	newnode->next = NODE_INT(NULL);

	do {
		// The object at the address corresponding to newnode->next
		// might be freed and reallocated at this point.  As of
		// early May 2019, there was some disagreement among WG14
		// committee members as to whether the C standard supports
		// newnode->next being converted back into a pointer to
		// a node_t structure in this case.
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, NODE_INT(newnode)));
}


void list_pop_all(void)
{
	struct node_t *p = NODE_PTR(atomic_exchange(&top, NODE_INT(NULL)));

	while (p) {
		struct node_t *next = NODE_PTR(p->next);

		foo(p);
		free(p);
		p = next;
	}
}

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
