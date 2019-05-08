PGMS = lifo-push lifo-push-atomic lifo-push-int lifo-push-london lifo-push-rcu lifo-push-rep

all: $(PGMS)

lifo-push: lifo-push.c lifo-stress.h
	cc -g -Wall -o lifo-push lifo-push.c -lpthread

lifo-push-atomic: lifo-push-atomic.c lifo-stress.h
	cc -g -Wall -o lifo-push-atomic lifo-push-atomic.c -lpthread

lifo-push-int: lifo-push-int.c lifo-stress.h
	cc -g -Wall -o lifo-push-int lifo-push-int.c -lpthread

lifo-push-london: lifo-push-london.c lifo-stress.h
	cc -g -Wall -o lifo-push-london lifo-push-london.c -lpthread

lifo-push-rcu: lifo-push-rcu.c lifo-stress.h
	cc -g -Wall -o lifo-push-rcu lifo-push-rcu.c -lpthread -lurcu -lurcu-signal

lifo-push-rep: lifo-push-rep.c lifo-stress.h
	cc -g -Wall -o lifo-push-rep lifo-push-rep.c -lpthread
