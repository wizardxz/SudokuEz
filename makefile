CXX = g++
CFLAGS = -Wall -Wconversion -O3 `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv`

all: main
main: main.o box.o feature.o processing.o solve.o
	$(CXX) $(CFLAGS) main.o box.o feature.o processing.o solve.o -o sudoku $(LIBS)
main.o:main.cpp
	$(CXX) $(CFLAGS) -c main.cpp
box.o:box.cpp box.h
	$(CXX) $(CFLAGS) -c box.cpp $(LIBS)
feature.o:feature.cpp
	$(CXX) $(CFLAGS) -c feature.cpp
processing.o:processing.cpp
	$(CXX) $(CFLAGS) -c processing.cpp
solve.o:solve.cpp
	$(CXX) $(CFLAGS) -c solve.cpp

clean:
	rm -f *.o
	rm -f sudoku 
