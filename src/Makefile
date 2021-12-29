.phony: all clean

all: re-keyboard

re-keyboard: main.o
	gcc -Wall -o $@ main.o `pkg-config --libs gtk+-3.0` -lm
	@echo "*** Success ***"

main.o: main.c
	gcc -Wall `pkg-config --cflags gtk+-3.0` -o main.o -c main.c

clean:
	-rm re-keyboard *.o
