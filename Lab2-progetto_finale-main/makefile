#flag per la compilazione
CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread


#per creare farm, client
all: farm client

client: client.o library.o
	$(CC) library.o client.o $(LDLIBS) -o client

client.o: client.c library.h
	$(CC) $(CFLAGS) -c -o client.o client.c

farm: farm.o library.o
	$(CC) library.o farm.o $(LDLIBS) -o farm

farm.o: farm.c library.h
	$(CC) $(CFLAGS) -c -o farm.o farm.c


library.o: library.c library.h
	$(CC) $(CFLAGS) -c -o library.o library.c



#zip del progetto
zip:
	zip progetto.zip makefile *.c *.h *.py *.md
