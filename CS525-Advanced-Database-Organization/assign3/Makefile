SRC_FILES = test_assign3_1.c expr.c record_mgr.c rm_serializer.c buffer_mgr_stat.c buffer_mgr.c dberror.c storage_mgr.c 
SRC_FILES_EXPR = test_expr.c expr.c record_mgr.c rm_serializer.c buffer_mgr_stat.c buffer_mgr.c dberror.c storage_mgr.c 

CC = gcc
CFLAGS = -Wall -O2

OBJECTS = $(SRC_FILES:%.c=%.o)

EXECS     = ${SRC_FILES:.c=}

all:
	gcc -o test_assign3 $(SRC_FILES)
	gcc -o test_assign3_expr $(SRC_FILES_EXPR)

clean:
	rm -f test_assign3
	rm -f test_assign3_expr

