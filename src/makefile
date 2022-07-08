#INC=-I/home/zezhao/workdir/partitioning_tool/metis-5.1.0/GKlib -I/home/zezhao/workdir/partitioning_tool/metis-install/include -I/home/zezhao/usr/local/googletest/googletest/include/
INC=-I/home/zezhao/usr/local/metis-5.1.0/include -I/home/zezhao/usr/local/gtest/include/
LIBS=-L/home/zezhao/usr/local/metis-5.1.0/lib/ -L/home/zezhao/usr/local/gtest/lib/

FLAGS=-std=c++17
FLAGS+=-O3 -g #-pg
CC=g++

all: mp

mp: main.o
	${CC} ${LIBS} ${FLAGS} main.o -o mp -lmetis -lgtest -lpthread 

main.o: main.cpp
	${CC} ${INC} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp
run: mp
	#export LD_LIBRARY_PATH=../metis-install/lib/:${LD_LIBRARY_PATH}
	#export LD_LIBRARY_PATH=/home/zezhao/usr/local/googletest/lib/:${LD_LIBRARY_PATH}
	#../metis-install/bin/mpmetis ien.dat 4
	./mp # ien.dat 4
