CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(CUDA_ROOT)/include
LDFLAGS=-L$(CUDA_ROOT)/lib64
LIBS=
SHARED_CXXFLAGS=-shared -fPIC

all: kp_nvtx_connector.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}../../common/makefile-only -I${MAKEFILE_PATH}../all

kp_nvtx_connector.so: ${MAKEFILE_PATH}kp_nvtx_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ ${MAKEFILE_PATH}kp_nvtx_connector.cpp $(LIBS)

clean:
	rm -f kp_nvtx_connector.so
