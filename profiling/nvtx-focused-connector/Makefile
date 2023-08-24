CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(CUDA_ROOT)/include -I./
LDFLAGS=-L$(CUDA_ROOT)/lib64
LIBS=-lnvToolsExt -lcudart
SHARED_CXXFLAGS=-shared -fPIC

all: kp_nvprof_focused_connector.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}../all

kp_nvprof_focused_connector.so: ${MAKEFILE_PATH}kp_nvprof_focused_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ ${MAKEFILE_PATH}kp_nvprof_focused_connector.cpp $(LIBS)

clean:
	rm *.so
