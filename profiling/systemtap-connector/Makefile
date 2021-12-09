CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

all: kp_systemtap_connector.so

${MAKEFILE_PATH}probes.h: ${MAKEFILE_PATH}probes.d
	dtrace -C -h -s $< -o $@

probes.o: ${MAKEFILE_PATH}probes.d
	dtrace -C -G -s $< -o $@

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_systemtap_connector.so: ${MAKEFILE_PATH}kp_systemtap_connector.cpp ${MAKEFILE_PATH}probes.h probes.o
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) probes.o \
		-o $@ ${MAKEFILE_PATH}kp_systemtap_connector.cpp $(LIBS)

clean:
	rm -rf *.so probes.h probes.o
