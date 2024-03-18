SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall -fopenmp

# CC=nvcc
# CFLAGS=-O3 -I$(HEADER_DIR) -Xcompiler -Wall -Xcompiler -fopenmp
LDFLAGS= 

SRC= apm.c

OBJ= $(OBJ_DIR)/apm.o

all: $(OBJ_DIR) apm

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^


# apm:$(OBJ)
# 	$(CC) $(CFLAGS) -o apm.c aux.c $(OBJ) $(LDFLAGS)

apm: obj/apm.o
	mpicc src/apm.c src/aux.c -o apm -O3 -Iinclude -Wall -fopenmp

clean:
	rm -f apm $(OBJ) ; rmdir $(OBJ_DIR)
out:
	rm -f *.out
