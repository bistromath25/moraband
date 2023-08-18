all:
	clang++ -pthread -w -mcpu=apple-m1 -std=c++17 -DNDEBUG -O3 *.cpp -o Moraband

run:
	clang++ -pthread -w -mcpu=apple-m1 -std=c++17 -DNDEBUG -O3 *.cpp -o Moraband
	./Moraband