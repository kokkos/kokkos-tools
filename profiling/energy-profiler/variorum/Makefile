#Variorum Dependancy
VAR_LIB=-L$(VARIORUM_ROOT)/lib
VAR_INC=-I$(VARIORUM_ROOT)/include
LINK_FLAG=-lvariorum

CXX=mpicxx
CXXFLAGS=-O3 -std=c++20 -g
SHARED_CXXFLAGS=-shared -fPIC
all: build/kp_power_variorum.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH} -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}/../all

SOURCES = kp_power_variorum.cpp \
          variorum_power_profiler.cpp

HEADERS = variorum_power_profiler.hpp

build/kp_power_variorum.so: $(SOURCES) $(HEADERS) | build
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(VAR_LIB) $(VAR_INC) -o $@ $(SOURCES) $(LINK_FLAG)

build:
	mkdir -p build

clean:
	rm -rf build/*.so
