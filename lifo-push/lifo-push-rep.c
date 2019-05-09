// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from lifo-push.c based on suggestions by Jens Gustedt
//	(substitute CAS for load) and Martin Sebor (casts to PointerRep).
//	Note that Martin has not yet indicated whether or not the code
//	in this file matches his intent.
//
// Absolutely -not- recommended for large-scale production use due to
// the loss of type checking.  You have been warned!!!

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

typedef char *value_t;

struct PointerRep {
	_Alignas (void *) unsigned char rep[sizeof(void *)];
};
const struct PointerRep NULLpr = { 0 };

struct node_t {
	value_t val;
	struct PointerRep next;
};

void set_value(struct node_t *p, value_t v)
{
	p->val = v;
}

void foo(struct node_t *p);

int list_empty(struct PointerRep p)
{
	return memcmp(&p, &NULLpr, sizeof(p)) == 0;
}
#define list_empty(p) list_empty(p)

// LIFO list structure
struct PointerRep _Atomic top;

void list_push(value_t v)
{
	struct node_t *newnode = (struct node_t *) malloc(sizeof(*newnode));
	struct PointerRep newnodepr;

	set_value(newnode, v);
	newnode->next = NULLpr;
	do {
		// The object at the address corresponding to newnode->next
                // might be freed and reallocated at this point.  As of
                // early May 2019, there was some disagreement among WG14
                // committee members as to whether the C standard supports
                // newnode->next being converted back into a pointer to
                // a node_t structure in this case.  Some members have
		// stated that a C implementation is permitted to modify
		// the representation bytes of an indeterminate pointer.
		memcpy(&newnodepr, &newnode, sizeof(newnodepr));
	} while (!atomic_compare_exchange_weak(&top, &newnode->next, newnodepr));
}


void list_pop_all()
{
	struct PointerRep ppr = atomic_exchange(&top, NULLpr);
	struct node_t *p;

	memcpy(&p, &ppr, sizeof(p));
	while (p) {
		struct PointerRep next;

		next = p->next;
		foo(p);
		free(p);
		memcpy(&p, &next, sizeof(p));
	}
}

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
