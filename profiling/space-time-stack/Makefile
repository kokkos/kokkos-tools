CXX=mpicxx
CXXFLAGS=-shared -O3 -g -fPIC -std=c++17 -Wall -Wextra

#Turn MPI support off:
#CXXFLAGS += -DUSE_MPI=0

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}../all -I${MAKEFILE_PATH}../../common

kp_space_time_stack.so: ${MAKEFILE_PATH}kp_space_time_stack.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.so 
