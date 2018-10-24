
CC = gcc
CFLAGS = -lpthread
CNAME = server_pthreads.c
EXECNAME = serverpthreads

pthreads_parallel:
	$(CC) -O3 -o $(EXECNAME) $(CNAME) $(CFLAGS)

clean:
	rm $(EXECNAME)

