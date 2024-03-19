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

all: $(OBJ_DIR) apm apm_cuda

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^


# apm:$(OBJ)
# 	$(CC) $(CFLAGS) -o apm.c aux.c $(OBJ) $(LDFLAGS)

apm_cuda :
	nvcc -c src/apm.cu -o obj/apm_cuda.o -Iinclude -O3 -Xcompiler -fopenmp

apm_autre: apm_cuda
	mpicc src/apm.c src/aux.c -o apm.o -O3 -Iinclude -Wall -fopenmp

apm : apm_autre
	mpicc src/apm.c src/aux.c obj/apm_cuda.o -o apm -O3 -Iinclude -Wall -fopenmp

clean:
	rm -f apm $(OBJ) ; rmdir $(OBJ_DIR)
out:
	rm -f *.out
