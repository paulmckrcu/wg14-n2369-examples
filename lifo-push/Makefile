PGMS = lifo-push lifo-push-int lifo-push-london

all: $(PGMS)

lifo-push: lifo-push.c lifo-stress.h
	cc -g -Wall -o lifo-push lifo-push.c -lpthread

lifo-push-int: lifo-push-int.c lifo-stress.h
	cc -g -Wall -o lifo-push-int lifo-push-int.c -lpthread

lifo-push-london: lifo-push-london.c lifo-stress.h
	cc -g -Wall -o lifo-push-london lifo-push-london.c -lpthread
