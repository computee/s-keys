CPPFLAGS+=-Iinclude
CFLAGS+=-O1

.PHONY: all
all: main

main: main.c jump_label.c jump_label_x86.c

.PHONY: clean
clean:
	rm -f main *.o *.s
