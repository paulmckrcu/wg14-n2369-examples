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
#include <assert.h>
#include <pthread.h>

#define N_HASH /* (1024 * 1024) */ 256
#include "shard-lock.h"

// Parts keyed by name and by ID.
struct part {
	int name;
	int id;
	int data;
};

struct part *nametab[N_HASH];
struct part *idtab[N_HASH];

struct part *delete_by_id(int id)
{
	int idhash = parthash(id);
	int namehash;
	struct part *partp = READ_ONCE(idtab[idhash]);

	if (!partp)
		return NULL;
	acquire_lock(partp);
	if (READ_ONCE(idtab[idhash]) == partp &&
	    partp->id == id) {
		namehash = parthash(partp->name);
		if (nametab[namehash] == partp)
			WRITE_ONCE(nametab[namehash], NULL);
		WRITE_ONCE(idtab[idhash], NULL);
	} else {
		partp = NULL;
	}
	release_lock(partp);
	return partp;
}

struct part *delete_by_name(int name)
{
	int idhash;
	int namehash = parthash(name);
	struct part *partp = READ_ONCE(nametab[namehash]);

	if (!partp)
		return NULL;
	acquire_lock(partp);
	if (READ_ONCE(nametab[namehash]) == partp &&
	    partp->name == name) {
		idhash = parthash(partp->id);
		if (idtab[idhash] == partp)
			WRITE_ONCE(idtab[idhash], NULL);
		WRITE_ONCE(nametab[namehash], NULL);
	} else {
		partp = NULL;
	}
	release_lock(partp);
	return partp;
}

int insert_part_by_id(struct part *partp)
{
	int idhash = parthash(partp->id);
	int ret = 0;

	acquire_lock(&idtab[idhash]);
	if (!idtab[idhash]) {
		WRITE_ONCE(idtab[idhash], partp);
		ret = 1;
	}
	release_lock(&idtab[idhash]);
	return ret;
}

int insert_part_by_name(struct part *partp)
{
	int namehash = parthash(partp->name);
	int ret = 0;

	acquire_lock(&nametab[namehash]);
	if (!nametab[namehash]) {
		WRITE_ONCE(nametab[namehash], partp);
		ret = 1;
	}
	release_lock(&nametab[namehash]);
	return ret;
}

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

int lookup_by_id(int id, struct part *partp_out)
{
	return lookup_by_bucket(idtab, &idtab[parthash(id)], partp_out);
}

int lookup_by_name(int name, struct part *partp_out)
{
	return lookup_by_bucket(nametab, &nametab[parthash(name)], partp_out);
}

void smoketest(void)
{
	struct part p0 = { .name = 5, .id = 10, .data = 42, };
	struct part p1 = { .name = 5, .id = 11, .data = 43, };
	struct part p2 = { .name = 6, .id = 10, .data = 44, };
	struct part p3 = { .name = 7, .id = 12, .data = 45, };
	struct part pout;

	assert(insert_part_by_id(&p0));
	assert(insert_part_by_name(&p0));
	assert(!insert_part_by_name(&p1));
	assert(!insert_part_by_id(&p2));
	assert(insert_part_by_id(&p3));
	assert(insert_part_by_name(&p3));

	assert(lookup_by_name(7, &pout));
	assert(pout.name == 7 && pout.id == 12);
	assert(!lookup_by_name(6, &pout));
	assert(pout.name == 7 && pout.id == 12);
	assert(lookup_by_id(10, &pout));
	assert(pout.name == 5 && pout.id == 10);
	assert(!lookup_by_id(11, &pout));
	assert(pout.name == 5 && pout.id == 10);

	assert(delete_by_id(10) == &p0);
	assert(!delete_by_id(11));
	assert(delete_by_name(7) == &p3);
	assert(!delete_by_name(6));
}

int main(int argc, char *argv[])
{
	smoketest();
	return 0;
}
