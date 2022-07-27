
include make.config

ifeq (${USE_OMPI}, 1)
export OMPI_CC=clang
export OMPI__CXX=clang++
CC=mpicc
CXX=mpicxx
endif

ifeq (${USE_MPICH}, 1)
CC=mpicc -cc=clang
CXX=mpicxx -cxx=clang++ 
endif

CC_DETAIL=$(shell ${CC} -show)
CXX_DETAIL=$(shell ${CXX} -show)

FLAGS=-std=c++17
FLAGS+=-O2 -g
#FLAGS+=-pg 
#FLAGS+=-fsanitize=address

LIBS=-lmetis -lgtest -lpthread -lhdf5 -lboost_program_options

all: mp

info:
	@echo "HOSTNAME ${HOSTNAME}"
	@echo "USE_OMPI: ${USE_OMPI}"
	@echo "USE_MPICH: ${USE_MPICH}"
	@echo "USE_INTEL: ${USE_INTEL}"
	@echo "CC: ${CC_DETAIL}"
	@echo "CXX: ${CXX_DETAIL}"


mp: main.o
	${CXX} ${FLAGS} main.o -o mp ${LIBS}

main.o: main.cpp
	${CXX} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp *.h5
