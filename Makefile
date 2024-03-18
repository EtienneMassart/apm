SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=nvcc 
CFLAGS=-O3 -I$(HEADER_DIR) -Xcompiler -Wall -Xcompiler -fopenmp
LDFLAGS= -lmpi

SRC= apm.cu

OBJ= $(OBJ_DIR)/apm.o

all: $(OBJ_DIR) apm

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cu
	$(CC) $(CFLAGS) -c -o $@ $^

apm:$(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f apm $(OBJ) ; rmdir $(OBJ_DIR)
out:
	rm -f *.out
