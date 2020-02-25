CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(PAPI_ROOT)/include
LDFLAGS=-L$(PAPI_ROOT)/lib
LIBS=-lpapi
SHARED_CXXFLAGS=-shared -fPIC

all: kp_papi_connector.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_papi_connector.so: ${MAKEFILE_PATH}kp_papi_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ ${MAKEFILE_PATH}kp_papi_connector.cpp $(LIBS)

clean:
	rm *.so
