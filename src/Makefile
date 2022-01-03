.phony: all clean

all: re-keyboard re-record

re-keyboard: main.o
	gcc -Wall -Werror -o $@ main.o `pkg-config --libs gtk+-3.0` -lm
	@echo "*** Success ***"

main.o: main.c
	gcc -Wall -Werror `pkg-config --cflags gtk+-3.0` -o main.o -c main.c

re-record: re-record.o
	gcc -Wall -Werror -o $@ $< `pkg-config --libs gtk+-3.0` -lm
	@echo "*** Success ***"

re-record.o: re-record.c
	gcc -Wall -Werror `pkg-config --cflags gtk+-3.0` -o $@ -c $<

clean:
	-rm re-keyboard re-record *.o
