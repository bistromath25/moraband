all:
	g++ -pthread -w -mcpu=apple-m1 -std=c++17 -O3 *.cpp -o Moraband
