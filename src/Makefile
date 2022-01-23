.phony: all clean

all: re-keyboard re-record re-draw re-read reGUI

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


re-draw: re-draw.o hershey-fonts.o
	gcc -Wall -Werror -o $@ $^ `pkg-config --libs gtk+-3.0` -lm
	@echo "*** Success ***"

re-draw.o: re-draw.c
	gcc -Wall -Werror `pkg-config --cflags gtk+-3.0` -o $@ -c $<

hershey-fonts.o: hershey-fonts.c
	gcc `pkg-config --cflags gtk+-3.0` -o $@ -c $<


re-read: re-read.o
	gcc -Wall -Werror -o $@ $< `pkg-config --libs gtk+-3.0` -lm
	@echo "*** Success ***"

re-read.o: re-read.c
	gcc -Wall -Werror `pkg-config --cflags gtk+-3.0` -o $@ -c $<


reGUI: re-gui.c
	gcc `pkg-config --cflags gtk4` -o $@ $< `pkg-config --libs gtk4`


clean:
	-rm re-keyboard re-record re-draw re-read reGUI *.o
