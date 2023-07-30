all:
	clang++ -pthread -w -mcpu=apple-m1 -std=c++17 -O3 *.cpp -o Moraband

run:
	clang++ -pthread -w -mcpu=apple-m1 -std=c++17 -O3 *.cpp -o Moraband
	./Moraband