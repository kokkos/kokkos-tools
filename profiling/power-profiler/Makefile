#Variorum Dependancy
VAR_LIB=-L$(VARIORUM_ROOT)/lib
VAR_INC=-I$(VARIORUM_ROOT)/include
LINK_FLAG=-lvariorum

CXX=mpicxx
CXXFLAGS=-O3 -std=c++20 -g
SHARED_CXXFLAGS=-shared -fPIC
all: power-profiler.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}../all
power-profiler.so: ${MAKEFILE_PATH}power-profiler.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(VAR_LIB) $(VAR_INC) -o $@ ${MAKEFILE_PATH}power-profiler.cpp $(LINK_FLAG)


clean:
	rm *.so
