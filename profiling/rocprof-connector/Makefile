CXX=g++
CXXFLAGS=-O3 -g -I$(ROCM_PATH)/roctracer/include
LDFLAGS=-L$(ROCM_PATH)/lib
LIBS=-lroctx64
SHARED_CXXFLAGS=-shared -fPIC

all: kp_rocprof_connector.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_rocprof_connector.so: ${MAKEFILE_PATH}kp_rocprof_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ ${MAKEFILE_PATH}kp_rocprof_connector.cpp $(LIBS)

clean:
	rm *.so
