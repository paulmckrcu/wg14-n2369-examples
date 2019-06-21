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

#define NODE_INT(x)	node_ptr2int(x)	
#define NODE_PTR(x)	node_int2ptr(x)

static inline struct node_ptr_t node_ptr2int(struct node_t *x)
{
	return (struct node_ptr_t){ (uintptr_t)x };
}

static inline struct node_t * node_int2ptr(struct node_ptr_t p)
{
	return (struct node_t *)p.i;
}

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
		/* In 'NODE_INT(x)' a pointer is cast to 'uintptr_t'. According to
		 * the proposal in N2362, the cast to uintptr_t in 'NODE_INT'
		 * would expose the storage instance at 'newnode' as an invisible
		 * side effect in the abstract machine. Consequently, all list
		 * nodes are always exposed.
		 *	
		 * Then 'newnode->next' is loaded by 'atomic_compare_exchang_weak'.
		 * If it were a pointer it may become invalid at any time if another
		 * thread calls 'list_pop_all()' and frees the object. All accesses
		 * to the pointer itself are then UB according to the current C
		 * standard. We store the pointer as an integer which can still be
		 * accessed.
		 *
		 * In case a new object happens to be allocated at the same place
		 * and is added to the top of the list, the representation of
		 * the integer (and 'struct node_ptr_t'*) for 'top' and 'next'
		 * compare equal. In this case, 'atomic_compare_exhange_weak'
		 * succeeds and overwrites 'top'. It points to a new object
		 * that would have an invalid pointer in 'next' in the
		 * traditional pointer-based implementation. Here it has
		 * an integer with the expected value (the abstract address
		 * of the object).
		 */

	} while (!atomic_compare_exchange_weak(&top, &newnode->next, NODE_INT(newnode)));
}


void list_pop_all(void)
{
	/* 'top' is always valid even in a pointer-based implementation,
	 * so nothing problematic happens here. The integer is converted
	 * back to an pointer ('NODE_PTR') which is guarantueed to work if
	 * 'uintptr_t' exists as a type. The rules in N2362 specify that this
	 * works if the integer corresponds to the address of an 'exposed' and
	 * life storage instance.
	 * */

	struct node_t *p = NODE_PTR(atomic_exchange(&top, NODE_INT(NULL)));

	while (p) {

		/* Here, the integer value is converted back. As the
		 * value of the integer always corresponds to an (abstract)
		 * address of an exposed storage instance, the conversion
		 * yields a valid pointer. This is the case even if the
		 * pointer it was originally derived from became invalid
		 * because the corresponding object was freed.
		 * */

		struct node_t *next = NODE_PTR(p->next);

		foo(p);
		free(p);
		p = next;
	}
}

#define rcu_register_thread() do { } while (0)
#define rcu_unregister_thread() do { } while (0)
#include "lifo-stress.h"
