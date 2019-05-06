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
	while (1) {
		struct node_t *p = atomic_load(&top);

		// p may have become invalid
		newnode->next = p; // May store invalid pointer that is
				   // dereferenced later but only if it
				   // is equal to a valid pointer.
		if (atomic_compare_exchange_weak(&top, &p, newnode))
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
