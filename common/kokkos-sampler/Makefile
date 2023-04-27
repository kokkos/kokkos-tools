CXX = clang++

CXXFLAGS = -O3 -std=c++17 -g

SHARED_CXXFLAGS= -shared -fPIC

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I../../profiling/all/ -I../makefile-only/

kp_sampler.so: ${MAKEFILE_PATH}kp_sampler_skip.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_sampler_skip.cpp

clean:
	rm *.so
