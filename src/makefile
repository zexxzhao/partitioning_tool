
CC=mpicc
CXX=mpicxx

FLAGS=-std=c++17
FLAGS+=-O2 -g
#FLAGS+=-pg 
#FLAGS+=-fsanitize=address


all: mp

mp: main.o
	${CXX} ${FLAGS} main.o -o mp -lmetis -lgtest -lpthread -lhdf5

main.o: main.cpp
	${CXX} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp *.h5
