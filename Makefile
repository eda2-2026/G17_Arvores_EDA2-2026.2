CC      ?= gcc
CFLAGS  += -Wall -O2 $(shell pkg-config --cflags raylib 2>/dev/null)
LDLIBS  += $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib) -lm

avl_visualizer: avl_visualizer.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

run: avl_visualizer
	./avl_visualizer

clean:
	rm -f avl_visualizer

.PHONY: run clean
