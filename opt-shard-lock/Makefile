PGMS = simp-opt-shard-lock

all: $(PGMS)

simp-opt-shard-lock: simp-opt-shard-lock.c shard-lock.h
	cc -g -Wall -o simp-opt-shard-lock simp-opt-shard-lock.c -lpthread

clean:
	rm *.o $(PGMS)
