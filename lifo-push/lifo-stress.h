////////////////////////////////////////////////////////////////////////
//
// Stress-test code
//
// Also serves as a crude high-contention performance test.
//
// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//	Adapted from similar tests in "Is Parallel Programming Hard,
//	And, If So, What Can You Do About It?":
//	git://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git

#define N_PUSH 2
#define N_POP  2
#define N_ELEM (10 * 1000 * 1000L)

char s[N_PUSH * N_ELEM];
int _Atomic goflag;

void foo(struct node_t *p)
{
	(*p->val)++;
}

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