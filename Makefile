all: clean obj_dir apm apm_seq apm_cuda

obj_dir:
	mkdir obj

apm_cuda.o:
	nvcc src/apm_cuda.cu -o obj/apm_cuda.o -Iinclude -O3 -Xcompiler -fopenmp

apm.o:
	mpicc src/apm.c src/aux.c -o obj/apm.o -O3 -Iinclude -Wall -fopenmp

apm_seq.o:
	gcc src/apm_seq.c -o obj/apm_seq.o -O3 -Iinclude

apm_cuda: apm_cuda.o
	nvcc src/apm_cuda.cu -o apm_cuda -Iinclude -O3 -Xcompiler -fopenmp

apm: apm.o
	mpicc src/apm.c src/aux.c -o apm -O3 -Iinclude -Wall -fopenmp

apm_seq: apm_seq.o
	gcc src/apm_seq.c -o apm_seq -O3 -Iinclude

clean:
	rm -f apm apm_cuda apm_seq ; rm -f -r obj
out:
	rm -f *.out
