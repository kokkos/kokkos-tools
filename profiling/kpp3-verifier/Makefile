CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_ecp_kpp3.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}


kp_ecp_kpp3.so: ${MAKEFILE_PATH}kp_ecp_kpp3.cpp ${MAKEFILE_PATH}kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_ecp_kpp3.cpp

clean:
	rm *.so
