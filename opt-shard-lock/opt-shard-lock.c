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

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define READ_ONCE(x) ({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })
#define WRITE_ONCE(x, val) do { ACCESS_ONCE(x) = (val); } while (0)

// Parts keyed by name and by ID.
struct part {
	int name;
	int id;
	int data;
};

#define N_HASH (1024 * 1024)
struct part *nametab[N_HASH]
struct part *idtab[N_HASH]

#define N_LOCK_SHARDS 16384
pthread_mutex_t shardlock[N_LOCK_SHARDS];

void init_shardlock(void)
{
	int i;

	for (i = 0; i < N_LOCK_SHARDS; i++)
		pthread_mutex_init(&shard_locks[i]);
}

int hash_lock(void *p)
{
	uintptr_t up = (uintptr_t) p;

	return (up / sizeof(up / sizeof(p)) % N_LOCK_SHARDS;
}

int parthash(int i)
{
	return i % N_HASH;
}

void acquire_lock(void *p)
{
	int i = hash_lock(p);

	assert(!pthread_mutex_lock(&shard_lock[i]);
}

void release_lock(void *p)
{
	int i = hash_lock(p);

	assert(!pthread_mutex_unlock(&shard_lock[i]));
}

void acquire_lock_pair(void *p1, void *p2)
{
	int i1 = hash_lock(p1);
	int i2 = hash_lock(p2);

	if (i1 < i2) {
		assert(!pthread_mutex_lock(&shard_lock[i1]);
		assert(!pthread_mutex_lock(&shard_lock[i2]);
	} else if (i2 < i1) {
		assert(!pthread_mutex_lock(&shard_lock[i2]);
		assert(!pthread_mutex_lock(&shard_lock[i1]);
	} else {
		assert(!pthread_mutex_lock(&shard_lock[i1]);
	}
}

void release_lock_pair(void *p1, void *p2)
{
	int i1 = hash_lock(p1);
	int i2 = hash_lock(p2);

	if (i1 < i2) {
		assert(!pthread_mutex_unlock(&shard_lock[i1]);
		assert(!pthread_mutex_unlock(&shard_lock[i2]);
	} else {
		assert(!pthread_mutex_unlock(&shard_lock[i1]);
	}
}

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
