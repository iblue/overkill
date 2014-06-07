CPP=g++
CFLAGS=-O3 -march=native -ggdb `pkg-config --cflags opencv`
LIBS=`pkg-config --libs opencv`
overkill: overkill.cpp
	$(CPP) $(CFLAGS) -o overkill overkill.cpp $(LIBS)

