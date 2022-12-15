

CXX=g++
CFLAGS=-shared -O3 -fPIC -std=c++11

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_hwm.so: ${MAKEFILE_PATH}kp_hwm.cpp
	$(CXX) $(CFLAGS) -o $@ $<

clean:
	rm *.so 
