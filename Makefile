
all: main

main: obj/main.o
	g++ -Wall -o ./bin/$@ $^ -lpthread

obj/main.o: src/main.cpp
	g++ -Wall -O2 -std=c++14 -march=native -mtune=native src/main.cpp -c -o obj/main.o

clean:
	rm -rf obj/*.o
	rm -rf bin/$(EXEC)