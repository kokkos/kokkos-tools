TIMEMORY_ROOT_DIR:=/usr/local
CXX=nvcc
CXXFLAGS=-O3 -std=c++11 -g -I$(TIMEMORY_ROOT_DIR)/include
LDFLAGS=-L$(TIMEMORY_ROOT_DIR)/lib -L$(TIMEMORY_ROOT_DIR)/lib64 -ltimemory
SHARED_CXXFLAGS=-shared --compiler-options=-fPIC -x cu

all: kp_timemory.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_timemory.so: ${MAKEFILE_PATH}kp_timemory.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_timemory.cpp $(LDFLAGS)

format:
	clang-format -i *.hpp *.cpp

clean:
	rm -f *.so
