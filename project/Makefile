
FLAG=-std=c++14 -O3 -m64 -g

all:
	g++ $(FLAG) FP_growth.cpp -o FP_growth

test: all
	./FP_growth 2 < data1.txt
	diff -u result.txt ans1.txt > diff.txt

test_ibm-5000: all
	./FP_growth 2 < ibm-5000.txt
	diff -u result.txt ans_ibm-5000.txt > diff.txt