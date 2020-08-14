CXX=nvcc
CXXFLAGS=-O0 -std=c++11 -g
SHARED_CXXFLAGS=-shared -Xcompiler -fPIC

all: kp_checkpoint.so reader

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

kp_checkpoint.so: ${MAKEFILE_PATH}kp_kernel_logger.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_kernel_logger.cpp

reader: ${MAKEFILE_PATH}reader.cpp
	$(CXX) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}reader.cpp
clean:
	rm *.so
