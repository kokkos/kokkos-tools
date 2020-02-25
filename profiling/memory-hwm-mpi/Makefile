CXX = mpicxx
CXXFLAGS = -shared -O3 -fPIC -std=c++11

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_hwm_mpi.so: ${MAKEFILE_PATH}kp_hwm_mpi.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.so 
