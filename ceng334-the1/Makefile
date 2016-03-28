.PHONY: default hw1 clean tar

OBJS = list.o main.o
CC = gcc
DEBUG = -g
CFLAGS = -Wall -c -std=c11
LFLAGS = -Wall

default: hw1

hw1: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o hw1

main.o : list.h
	$(CC) $(CFLAGS) main.c

list.o : list.h
	$(CC) $(CFLAGS) list.c

clean:
	\rm *.o *~ hw1

tar:
	tar -cv list.h main.c list.c Makefile | gzip > hw1.tar.gz
