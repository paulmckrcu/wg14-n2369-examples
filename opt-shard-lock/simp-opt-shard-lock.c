// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//      Adapted from pseudocode in WG14 N2369.
//
// Simplified beyond belief:
// - Only one item in each hash bucket, which matches well-tuned common case.
// - Integer name and ID to trivialize hash functions.
// - Hash functions trivial even given integer trivialization.
// - Parts added to hash tables one at a time:  Removal from all tables
//   is atomic, but addition is sequential.  Pathetic rationale: Names
//   might be assigned by Marketing late in the game.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>

#define N_HASH /* (1024 * 1024) */ 256
#include "shard-lock.h"

// Parts keyed by name and by ID.
struct part {
	int name;
	int id;
	int data;
	int namestate; // 0=out, 1=in
	int idstate; // 0=out, 1=in
	struct part *statp; // Pointer to statically allocated shadow
};

struct part *nametab[N_HASH];
struct part *idtab[N_HASH];

// Delete from all tables, return pointer to part or NULL if not present
struct part *delete_by_id(int id)
{
	int idhash = parthash(id);
	int namehash;
	struct part *partp = READ_ONCE(idtab[idhash]);

	if (!partp)
		return NULL;
	acquire_lock(partp);
	if (READ_ONCE(idtab[idhash]) == partp && partp->id == id) {
		namehash = parthash(partp->name);
		if (nametab[namehash] == partp)
			WRITE_ONCE(nametab[namehash], NULL);
		WRITE_ONCE(idtab[idhash], NULL);
		release_lock(partp);
	} else {
		release_lock(partp);
		partp = NULL;
	}
	return partp;
}

// Delete from all tables, return pointer to part or NULL if not present
struct part *delete_by_name(int name)
{
	int idhash;
	int namehash = parthash(name);
	struct part *partp = READ_ONCE(nametab[namehash]);

	if (!partp)
		return NULL;
	acquire_lock(partp);
	if (READ_ONCE(nametab[namehash]) == partp && partp->name == name) {
		idhash = parthash(partp->id);
		if (idtab[idhash] == partp)
			WRITE_ONCE(idtab[idhash], NULL);
		WRITE_ONCE(nametab[namehash], NULL);
		release_lock(partp);
	} else {
		release_lock(partp);
		partp = NULL;
	}
	return partp;
}

// Insertion helper function
int insert_part_by_bucket(struct part **bkt, struct part *partp)
{
	int ret = 0;

	acquire_lock_pair(bkt, partp);
	if (!*bkt) {
		WRITE_ONCE(*bkt, partp);
		ret = 1;
	}
	release_lock_pair(bkt, partp);
	return ret;
}

// Insert specified part by its ID, return true if successful
int insert_part_by_id(struct part *partp)
{
	return insert_part_by_bucket(&idtab[parthash(partp->id)], partp);
}

// Insert specified part by its name, return true if successful
int insert_part_by_name(struct part *partp)
{
	return insert_part_by_bucket(&nametab[parthash(partp->name)], partp);
}

// Lookup helper function
int lookup_by_bucket(struct part **tab, struct part **bkt,
		     struct part *partp_out)
{
	int hash = bkt - &tab[0];
	struct part *partp = READ_ONCE(tab[hash]);
	int ret = 0;

	if (!partp)
		return 0;
	acquire_lock(partp);
	if (partp == tab[hash]) {
		*partp_out = *partp;
		ret = 1;
	}
	release_lock(partp);
	return ret;
}

// Lookup part by ID, copying it out and returning true if found
int lookup_by_id(int id, struct part *partp)
{
	int ret = lookup_by_bucket(idtab, &idtab[parthash(id)], partp);

	if (partp->id == id)
		return ret;
	return 0;
}

// Lookup part by name, copying it out and returning true if found
int lookup_by_name(int name, struct part *partp)
{
	int ret = lookup_by_bucket(nametab, &nametab[parthash(name)], partp);

	if (partp->name == name)
		return ret;
	return 0;
}

int alloc_and_insert_part_by_id(struct part *p)
{
	struct part *q = p->statp;

	if (!q)
		q = malloc(sizeof(*q));
	assert(q);
	*q = *p;
	p->statp = q;
	q->statp = p;
	return insert_part_by_id(q);
}

int alloc_and_insert_part_by_name(struct part *p)
{
	struct part *q = p->statp;

	if (!q)
		q = malloc(sizeof(*q));
	assert(q);
	*q = *p;
	p->statp = q;
	q->statp = p;
	return insert_part_by_name(q);
}

struct part *delete_and_free_by_id(int id)
{
	struct part *q = delete_by_id(id);
	struct part *p;

	if (!q)
		return NULL;
	p = q->statp;
	free(q);
	p->statp = NULL;
	return p;
}

struct part *delete_and_free_by_name(int name)
{
	struct part *q = delete_by_name(name);
	struct part *p;

	if (!q)
		return NULL;
	p = q->statp;

	free(q);
	p->statp = NULL;
	return p;
}

int nthreads = 4;
int partsperthread = 1000;
int _Atomic goflag;

void *stress_shard(void *arg)
{
	uintptr_t count = 0;
	int i;
	struct part *partbase = (struct part *)arg;

	printf("%s: partbase: %p\n", __func__, partbase);
	while (!atomic_load(&goflag))
		continue;
	while (atomic_load(&goflag) < 2) {
		for (i = 0; i < partsperthread; i++) {
			struct part *p = &partbase[i];
			struct part part_out;
			int state;

			part_out.name = 0;
			part_out.id = 0;
			part_out.data = 0;
			part_out.namestate = 0;
			part_out.idstate = 0;
			part_out.statp = NULL;
			state = lookup_by_id(p->id, &part_out);
			assert(state == 0 || state == 1);
			assert(p->idstate == 0 || p->idstate == 1);
			assert(state == p->idstate);
			if (p->idstate) {
				assert(p->name == part_out.name);
				assert(p->id == part_out.id);
				assert(p->data == part_out.data);
				assert(p == part_out.statp);
			}
			state = lookup_by_name(p->name, &part_out);
			assert(state == 0 || state == 1);
			assert(p->namestate == 0 || p->namestate == 1);
			assert(state == p->namestate);
			if (p->namestate) {
				assert(p->name == part_out.name);
				assert(p->id == part_out.id);
				assert(p->data == part_out.data);
				assert(p == part_out.statp);
			}
			if (!p->idstate && alloc_and_insert_part_by_id(p)) {
				assert(lookup_by_id(p->id, &part_out));
				p->idstate = 1;
				continue;
			} else if (!p->idstate) {
				continue; // Couldn't insert
			} else if (!p->namestate &&
				   alloc_and_insert_part_by_name(p)) {
				assert(lookup_by_name(p->name, &part_out));
				p->namestate = 1;
				continue;
			} else if (!p->namestate) {
				continue;
			} else if (i & 0x1) {
				assert(delete_and_free_by_id(p->id) == p);
				assert(!lookup_by_id(p->id, &part_out));
				assert(!p->statp);
				p->idstate = 0;
				p->namestate = 0;
			} else {
				assert(delete_and_free_by_name(p->name) == p);
				assert(!lookup_by_name(p->name, &part_out));
				assert(!p->statp);
				p->idstate = 0;
				p->namestate = 0;
			}
		}
		count++;
	}
	return (void *)count;
}

void stresstest(void)
{
	int i;
	struct part *partbin;
	pthread_t *tidp;
	void *vp;

	printf("Starting stress test.\n");
	partbin = malloc(sizeof(*partbin) * nthreads * partsperthread);
	tidp = malloc(sizeof(*tidp) * nthreads);
	for (i = 0; i < nthreads * partsperthread; i++) {
		partbin[i].name = i;
		partbin[i].id = 3 * i;
		partbin[i].data = 7 * i;
		partbin[i].namestate = 0;
		partbin[i].idstate = 0;
		partbin[i].statp = NULL;
	}
	for (i = 0; i < nthreads; i++) {
		if (pthread_create(&tidp[i], NULL, stress_shard,
				   (void *)&partbin[i * partsperthread])) {
			perror("pthread_create");
			exit(1);
		}
	}
	atomic_store(&goflag, 1);
	poll(NULL, 0, 10000);
	atomic_store(&goflag, 2);
	for (i = 0; i < nthreads; i++) {
		if (pthread_join(tidp[i], &vp)) {
			perror("pthread_join");
			exit(1);
		}
		printf("Thread %d # loops: %lu\n", i, (uintptr_t)vp);
	}
	for (i = 0; i < nthreads * partsperthread; i++)
		free(partbin[i].statp);
	free(partbin);
	free(tidp);
}

void smoketest(void)
{
	struct part p0 = { .name = 5, .id = 10, .data = 42, };
	struct part p1 = { .name = 5, .id = 11, .data = 43, };
	struct part p2 = { .name = 6, .id = 10, .data = 44, };
	struct part p3 = { .name = 7, .id = 12, .data = 45, };
	struct part pout;

	printf("Starting smoke test.\n");
	assert(insert_part_by_id(&p0));
	assert(insert_part_by_name(&p0));
	assert(!insert_part_by_name(&p1));
	assert(lookup_by_name(5, &pout));
	assert(!insert_part_by_id(&p2));
	assert(insert_part_by_id(&p3));
	assert(insert_part_by_name(&p3));

	assert(lookup_by_name(7, &pout));
	assert(pout.name == 7 && pout.id == 12);
	assert(!lookup_by_name(7 + N_HASH, &pout));
	assert(!lookup_by_name(6, &pout));
	assert(lookup_by_id(10, &pout));
	assert(pout.name == 5 && pout.id == 10);
	assert(!lookup_by_id(11, &pout));

	assert(delete_by_id(10) == &p0);
	assert(!delete_by_id(11));
	assert(delete_by_name(7) == &p3);
	assert(!delete_by_name(6));

	printf("Starting malloc()/free() smoke test.\n");
	assert(alloc_and_insert_part_by_id(&p0));
	assert(alloc_and_insert_part_by_name(&p0));
	assert(!alloc_and_insert_part_by_name(&p1));
	assert(lookup_by_name(5, &pout));
	assert(!alloc_and_insert_part_by_id(&p2));
	assert(alloc_and_insert_part_by_id(&p3));
	assert(alloc_and_insert_part_by_name(&p3));

	assert(lookup_by_name(7, &pout));
	assert(pout.name == 7 && pout.id == 12);
	assert(!lookup_by_name(7 + N_HASH, &pout));
	assert(!lookup_by_name(6, &pout));
	assert(lookup_by_id(10, &pout));
	assert(pout.name == 5 && pout.id == 10);
	assert(!lookup_by_id(11, &pout));

	assert(delete_and_free_by_id(10) == &p0);
	assert(!delete_and_free_by_id(11));
	assert(delete_and_free_by_name(7) == &p3);
	assert(!delete_and_free_by_name(6));
}

int main(int argc, char *argv[])
{
	smoketest();
	stresstest();
	return 0;
}
