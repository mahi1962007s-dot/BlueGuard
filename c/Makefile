CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
SRC     = main.c zone.c health_score.c array_ops.c search.c sorting.c linked_list.c stack.c queue.c ny_dataset.c advanced.c

all: beachdsa

beachdsa: $(SRC)
	$(CC) $(CFLAGS) -o beachdsa $(SRC)

clean:
	rm -f beachdsa beachdsa.exe

.PHONY: all clean
