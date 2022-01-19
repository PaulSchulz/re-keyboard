.phony: all src clean

all: src

src:
	make -C $<

clean: src doc
	make -C $< clean
