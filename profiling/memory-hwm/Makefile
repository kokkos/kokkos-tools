

CXX=g++
CXXFLAGS=-shared -O3 -fPIC -std=c++11

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}../all

kp_hwm.so: ${MAKEFILE_PATH}kp_hwm.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.so 
