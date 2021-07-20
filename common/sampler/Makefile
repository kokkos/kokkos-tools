CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I../../include
SHARED_CXXFLAGS=-shared -fPIC

all: kp_sampler.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

kp_sampler.so: ${MAKEFILE_PATH}sampler.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}sampler.cpp

clean:
	rm *.so
