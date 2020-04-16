CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -fopenmp
SHARED_CXXFLAGS=-shared -fPIC

all: kp_memory_usage.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_memory_usage.so: ${MAKEFILE_PATH}kp_memory_usage.cpp ${MAKEFILE_PATH}kp_memory_events.hpp ${MAKEFILE_PATH}kp_timer.hpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_memory_usage.cpp

clean:
	rm *.so
