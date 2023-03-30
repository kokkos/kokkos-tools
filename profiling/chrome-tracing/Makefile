CXX=mpicxx
CXXFLAGS=-shared -O3 -g -fPIC -std=c++11 -Wall -Wextra

#Turn MPI support off:
#CXXFLAGS += -DUSE_MPI=0

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}../all

kp_chrome_tracing.so: ${MAKEFILE_PATH}kp_chrome_tracing.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.so 
