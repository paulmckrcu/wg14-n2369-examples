PGMS = lifo-push lifo-push-london

all: $(PGMS)

lifo-push: lifo-push.c fifo-stress.h
	cc -g -Wall -o lifo-push lifo-push.c -lpthread

lifo-push-london: lifo-push-london.c fifo-stress.h
	cc -g -Wall -o lifo-push-london lifo-push-london.c -lpthread
