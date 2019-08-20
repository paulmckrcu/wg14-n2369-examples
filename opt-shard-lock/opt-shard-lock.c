// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//      Adapted from pseudocode in WG14 N2369.
//
// Simplified beyond belief:
// - Only one item in each hash bucket, which matches well-tuned common case.
// - Integer name and ID to trivialize hash functions.
// - Hash functions trivial even given integer trivialization.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "shard-lock.h"

// Parts keyed by name and by ID.
struct part {
	int name;
	int id;
	int data;
};

#define N_HASH (1024 * 1024)
struct part *nametab[N_HASH]
struct part *idtab[N_HASH]

struct part *delete_by_name(int name)
{
	int idhash;
	int namehash = parthash(name);
	struct part *partp = READ_ONCE(nametab[name]);

	if (!partp)
		return NULL;
	acquire_lock(partp);
	if (READ_ONCE(nametab[namehash]) == partp &&
	    partp->name == name) {
		idhash = parthash[partp->id];
		assert(idtab[idhash] == partp);
		WRITE_ONCE(idtab[idhash], NULL);
		WRITE_ONCE(nametab[namehash], NULL);
	} else {
		partp = NULL;
	}
	release_lock(partp);
	return partp;
}

int insert_part(struct part *partp)
{
	int idhash = parthash(partp->id);
	int namehash = parthash(partp->name);
	int ret = 0;

	acquire_lock_pair(&idtab[idhash], &nametab[namehash]);
	if (!idtab[idhash] && !nametab[namehash]) {
		WRITE_ONCE(idtab[idhash], partp);
		WRITE_ONCE(nametab[namehash], partp);
		ret = 1;
	}
	release_lock_pair(&idtab[idhash], &nametab[namehash]);
}

int lookup_by_id(int id, struct part *partp_out)
{
	int idhash = parthash(id);
	struct part *partp = READ_ONCE(idtable[idhash]);
	int ret = 0;

	acquire_lock(partp);
	if (partp == idtab[idhash]) {
		*partp_out = *partp;
		ret = 1;
	}
	release_lock(partp);
	return ret;
}
