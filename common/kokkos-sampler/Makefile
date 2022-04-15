CXX=g++
CXXFLAGS=-O3 -std=c++11 -g 

SHARED_CXXFLAGS=-shared -fPIC
LIBS=-lrt

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_sampler.so: ${MAKEFILE_PATH}kp_sampler.cpp ${MAKEFILE_PATH}kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ ${MAKEFILE_PATH}kp_sampler.cpp \
		$(LIBS)	

clean:
	rm *.so
