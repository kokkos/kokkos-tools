CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_systemtap_connector.so

probes.h: ${MAKEFILE_PATH}probes.d
	dtrace -C -h -s $< -o $@

probes.o: ${MAKEFILE_PATH}probes.d
	dtrace -C -G -s $< -o $@

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_systemtap_connector.so: ${MAKEFILE_PATH}kp_systemtap_connector.cpp ${MAKEFILE_PATH}probes.h probes.o
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) probes.o \
		-o $@ ${MAKEFILE_PATH}kp_systemtap_connector.cpp $(LIBS)

clean:
	rm -rf *.so probes.h probes.o
