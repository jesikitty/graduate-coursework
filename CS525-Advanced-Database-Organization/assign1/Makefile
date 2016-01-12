SRC_FILES = test_assign1_1.c dberror.c storage_mgr.c
SRC_FILES_EXTRA = test_assign1_extra.c dberror.c storage_mgr.c

CC = gcc
CFLAGS = -Wall -O2

OBJECTS = $(SRC_FILES:%.c=%.o)

EXECS     = ${SRC_FILES:.c=}

all:
	gcc -o test_assign1 $(SRC_FILES)
	gcc -o test_assign1_extra $(SRC_FILES_EXTRA)

clean:
	rm -f test_assign1
	rm -f test_assign1_extra