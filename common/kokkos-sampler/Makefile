CXX=g++
CXXFLAGS=-O3 -std=c++11 -g 

SHARED_CXXFLAGS=-shared -fPIC

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_sampler.so: ${MAKEFILE_PATH}kp_sampler.cpp
       $(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_sampler.cpp

clean:
	rm *.so
