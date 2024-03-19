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

all: $(OBJ_DIR) apm apm_seq apm_cuda

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

apm_cuda:
	nvcc src/apm_cuda.cu -o apm_cuda -Iinclude -O3 -Xcompiler -fopenmp

apm :
	mpicc src/apm.c src/aux.c -o apm -O3 -Iinclude -Wall -fopenmp

apm_seq :
	gcc src/apm_seq.c -o apm_seq -O3 -Iinclude

clean:
	rm -f apm $(OBJ) ; rmdir $(OBJ_DIR)
out:
	rm -f *.out
