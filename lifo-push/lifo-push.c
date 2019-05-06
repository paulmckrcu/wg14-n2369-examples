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

////////////////////////////////////////////////////////////////////////
//
// Test code

#define N_PUSH 2
#define N_POP  2
#define N_ELEM (1000 * 1000 * 1000L)

char s[N_PUSH * N_ELEM];
int _Atomic goflag;

void *push_em(void *arg)
{
	long i;
	char *my_s = arg;

	while (!atomic_load(&goflag))
		continue;
	for (i = 0; i < N_ELEM; i++)
		list_push(&my_s[i]);
	return NULL;
}

void *pop_em(void *arg)
{
	while (!atomic_load(&goflag))
		continue;
	while (atomic_load(&goflag) == 1)
		list_pop_all();
	return NULL;
}

int main(int argc, char *argv[])
{
	long i;
	pthread_t tid[N_PUSH + N_POP];
	void *vp;

	for (i = 0; i < N_PUSH; i++)
		if (pthread_create(&tid[i], NULL, push_em, (void *)&s[N_ELEM * i])) {
			perror("pthread_create");
			exit(1);
		}
	for (i = 0; i < N_POP; i++)
		if (pthread_create(&tid[N_PUSH + i], NULL, pop_em, NULL)) {
			perror("pthread_create");
			exit(1);
		}
	atomic_store(&goflag, 1);
	for (i = 0; i < N_PUSH; i++)
		if (pthread_join(tid[i], &vp) != 0) {
			perror("pthread_join");
			exit(1);
		}
	while (atomic_load(&top))
		sleep(1);
	atomic_store(&goflag, 2);
	for (i = 0; i < N_POP; i++)
		if (pthread_join(tid[N_PUSH + i], &vp) != 0) {
			perror("pthread_join");
			exit(1);
		}
	for (i = 0; i < N_PUSH * N_ELEM; i++)
		if (s[i] != 1) {
			fprintf(stderr, "Entry %ld left set\n", i);
			abort();
		}
}
