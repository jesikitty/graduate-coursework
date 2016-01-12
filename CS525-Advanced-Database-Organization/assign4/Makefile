SRC_FILES = test_assign4_1.c btree_mgr.c expr.c record_mgr.c rm_serializer.c buffer_mgr_stat.c buffer_mgr.c dberror.c storage_mgr.c 

CC = gcc
CFLAGS = -Wall -O2

OBJECTS = $(SRC_FILES:%.c=%.o)

EXECS     = ${SRC_FILES:.c=}

all:
	gcc -o test_assign4 $(SRC_FILES)

clean:
	rm -f test_assign4

