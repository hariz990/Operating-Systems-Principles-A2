.default: output
output: main.o allocate.o
	g++ -Wall -pedantic -std=c++17 Main.o -o allocate -pthread

main.o: Main.cpp
	g++ -Wall -pedantic -std=c++17 -c Main.cpp -g -pthread

allocate.o: Allocation.cpp
	g++ -Wall -pedantic -std=c++17 -c Allocation.cpp -pthread

clean:
	rm *.o allocate