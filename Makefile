CC=gcc
CFLAGS=-std=c99 -march=native -ggdb `pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`
overkill: overkill.c
	$(CC) $(CFLAGS) -o overkill overkill.c $(LIBS)

.PHONY: clean
clean:
	rm -f overkill


