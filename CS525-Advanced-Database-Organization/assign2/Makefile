SRC_FILES = test_assign2_1.c buffer_mgr_stat.c buffer_mgr.c dberror.c storage_mgr.c 

CC = gcc
CFLAGS = -Wall -O2

OBJECTS = $(SRC_FILES:%.c=%.o)

EXECS     = ${SRC_FILES:.c=}

all:
	gcc -o test_assign2 $(SRC_FILES)

clean:
	rm -f test_assign2
