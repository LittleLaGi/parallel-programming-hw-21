# /HW4/data/
# data1_1  data1_2  data1_3  data1_4  data1_5  data1_6  data2_1  data2_2  data2_3  data2_4  data2_5  data2_6

SUFFIX=-I/usr/lib/x86_64-linux-gnu/openmpi/include/openmpi -I/usr/lib/x86_64-linux-gnu/openmpi/include -pthread -L/usr/lib/x86_64-linux-gnu/openmpi/lib -lmpi_cxx -lmpi
FLAG=-O3 -m64

all:
	g++ $(FLAG) main.cc matmul.cc -o matmul $(SUFFIX)

test: all
	mpirun -np 4 --hostfile hosts matmul < test

run: all
	mpirun -np 4 --hostfile hosts matmul < /HW4/data/data2_3

clean:
	rm -f matmul