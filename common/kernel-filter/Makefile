CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_kernel_filter.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

kp_kernel_filter.so: ${MAKEFILE_PATH}kp_kernel_filter.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_kernel_filter.cpp

clean:
	rm *.so
