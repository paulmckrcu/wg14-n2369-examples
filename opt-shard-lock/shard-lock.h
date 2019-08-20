// Copyright IBM Corporation, 2019
// Authors: Paul E. McKenney, IBM Linux Technology Center
//
// Simplified beyond belief:
// - Hash functions trivial beyond belief.

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define READ_ONCE(x) ({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })
#define WRITE_ONCE(x, val) do { ACCESS_ONCE(x) = (val); } while (0)

#define N_LOCK_SHARDS 16384
pthread_mutex_t shard_lock[N_LOCK_SHARDS];

void init_shardlock(void)
{
	int i;

	for (i = 0; i < N_LOCK_SHARDS; i++)
		pthread_mutex_init(&shard_lock[i], NULL);
}

int hash_lock(void *p)
{
	uintptr_t up = (uintptr_t) p;

	return (up / sizeof(up / sizeof(p))) % N_LOCK_SHARDS;
}

int parthash(int i)
{
	return i % N_HASH;
}

void acquire_lock(void *p)
{
	int i = hash_lock(p);

	assert(!pthread_mutex_lock(&shard_lock[i]));
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
		assert(!pthread_mutex_lock(&shard_lock[i1]));
		assert(!pthread_mutex_lock(&shard_lock[i2]));
	} else if (i2 < i1) {
		assert(!pthread_mutex_lock(&shard_lock[i2]));
		assert(!pthread_mutex_lock(&shard_lock[i1]));
	} else {
		assert(!pthread_mutex_lock(&shard_lock[i1]));
	}
}

void release_lock_pair(void *p1, void *p2)
{
	int i1 = hash_lock(p1);
	int i2 = hash_lock(p2);

	if (i1 < i2) {
		assert(!pthread_mutex_unlock(&shard_lock[i1]));
		assert(!pthread_mutex_unlock(&shard_lock[i2]));
	} else {
		assert(!pthread_mutex_unlock(&shard_lock[i1]));
	}
}
