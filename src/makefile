#INC=-I/home/zezhao/workdir/partitioning_tool/metis-5.1.0/GKlib -I/home/zezhao/workdir/partitioning_tool/metis-install/include -I/home/zezhao/usr/local/googletest/googletest/include/
#INC=-I/home/zezhao/usr/local/metis-5.1.0/include -I/home/zezhao/usr/local/gtest/include/
#LIBS=-L/home/zezhao/usr/local/metis-5.1.0/lib/ -L/home/zezhao/usr/local/gtest/lib/

FLAGS=-std=c++17
FLAGS+=-O3 -g 
#FLAGS+=-pg 
#FLAGS+=-fsanitize=address
CC=g++

all: mp

mp: main.o
	${CC} ${FLAGS} main.o -o mp -lmetis -lgtest -lpthread 

main.o: main.cpp
	${CC} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp
