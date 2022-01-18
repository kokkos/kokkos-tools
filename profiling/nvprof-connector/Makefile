CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(CUDA_ROOT)/include
LDFLAGS=-L$(CUDA_ROOT)/lib64
LIBS=-lnvToolsExt
SHARED_CXXFLAGS=-shared -fPIC

all: kp_nvprof_connector.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_nvprof_connector.so: ${MAKEFILE_PATH}kp_nvprof_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ ${MAKEFILE_PATH}kp_nvprof_connector.cpp $(LIBS)

clean:
	rm *.so
