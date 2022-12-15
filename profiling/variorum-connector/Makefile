# Author: Zachary S. Frye
#Organization: CASC at LLNL
#Date: July 2020
#Description: This is a simple makefile for testing and developing the Kokkos-variorum connector


#Variorum stuff
VAR_LIB=-L$(VARIORUM_ROOT)/lib
VAR_INC=-I$(VARIORUM_ROOT)/include
LINK_FLAG=-lvariorum

CXX=mpicxx
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC
CFLAGS=-std=c++11 -Wall -g
all: variorum_connector.so



MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

variorum_connector.so: ${MAKEFILE_PATH}variorum-connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(VAR_LIB) $(VAR_INC) -o $@ ${MAKEFILE_PATH}variorum-connector.cpp $(LINK_FLAG)


clean:
	rm *.so
