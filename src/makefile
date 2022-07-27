
include make.config

all: mp

info:
	@echo "HOSTNAME: ${HOSTNAME}"
	@echo "SYS: ${SYS}"
	@#echo "USE_OMPI: ${USE_OMPI}"
	@#echo "USE_MPICH: ${USE_MPICH}"
	@#echo "USE_INTEL: ${USE_INTEL}"
	@echo "CC: '${CC}'='${CC_DETAIL}'"
	@echo "CXX: '${CXX}'='${CXX_DETAIL}'"


mp: main.o
	${CXX} ${FLAGS} main.o -o mp ${LINK} ${LIBS}

main.o: main.cpp
	${CXX} ${FLAGS} -MMD -c main.cpp

-include main.d
clean:
	rm *.o *.d mp *.h5
