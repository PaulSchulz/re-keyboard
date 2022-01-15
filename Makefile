.phony: all clean

all: src
	make -C $<

clean: src
	make -C $< clean
