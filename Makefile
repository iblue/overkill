CC=gcc
CFLAGS=-std=c99 -march=native -ggdb `pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`
overkill: overkill.o features.o
	$(CC) $(CFLAGS) -o overkill overkill.o features.o $(LIBS)

overkill.o: overkill.c
	$(CC) $(CFLAGS) -o overkill.o -c overkill.c

features.o: features.c features.h
	$(CC) $(CFLAGS) -o features.o -c features.c

.PHONY: clean
clean:
	rm -f overkill features.o overkill.o


