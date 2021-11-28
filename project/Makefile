
FLAG=-std=c++14 -O3 -m64 -g

all:
	g++ $(FLAG) FP_growth.cpp -o FP_growth

test: all
	./FP_growth 2 < data1.txt
	diff -u result.txt ans1.txt > diff.txt
