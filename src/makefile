
CC=mpicc -cc=clang
CXX=mpicxx -cxx=clang++ 
FLAGS=-std=c++17
FLAGS+=-O2 -g
#FLAGS+=-pg 
#FLAGS+=-fsanitize=address

LIBS=-lmetis -lgtest -lpthread -lhdf5 -lboost_program_options

all: mp

mp: main.o
	${CXX} ${FLAGS} main.o -o mp ${LIBS}

main.o: main.cpp
	${CXX} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp *.h5
