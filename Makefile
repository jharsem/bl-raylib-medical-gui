CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99
LDLIBS  := $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib) -lm -lpthread -ldl

demo: demo.c raymed.h
	$(CC) $(CFLAGS) -o $@ demo.c $(LDLIBS)

clean:
	rm -f demo

.PHONY: clean
