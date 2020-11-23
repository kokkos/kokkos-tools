CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_kernel_timer.so kp_reader kp_json_writer

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}

kp_reader: ${MAKEFILE_PATH}kp_reader.cpp kp_kernel_timer.so
	$(CXX) $(CXXFLAGS) -o kp_reader ${MAKEFILE_PATH}kp_reader.cpp

kp_json_writer: ${MAKEFILE_PATH}kp_json_writer.cpp kp_kernel_timer.so
	$(CXX) $(CXXFLAGS) -o kp_json_writer ${MAKEFILE_PATH}kp_json_writer.cpp

kp_kernel_timer.so: ${MAKEFILE_PATH}kp_kernel_timer.cpp ${MAKEFILE_PATH}kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ ${MAKEFILE_PATH}kp_kernel_timer.cpp

clean:
	rm *.so kp_reader
