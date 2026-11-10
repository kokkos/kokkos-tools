# User must set the path to OVIS install (required)
ifndef OVIS_DIR
$(error OVIS_DIR is not set. Please set OVIS_DIR to your OVIS installation path)
endif

# User must set the path to Kokkos Tools install (required)
ifndef PATH_TO_KOKKOS_TOOLS_INSTALL
$(error PATH_TO_KOKKOS_TOOLS_INSTALL is not set. Please set PATH_TO_KOKKOS_TOOLS_INSTALL to your Kokkos Tools installation path)
endif

KTO_DIR=${PATH_TO_KOKKOS_TOOLS_INSTALL}/kokkos-tools/
CXX?=g++
CXXFLAGS=-O3 -std=c++11 -g \
	-I$(OVIS_DIR)/include/ -I./include -I$(KTO_DIR)/profiling/all/ -I$(KTO_DIR)/common/makefile-only/

SHARED_CXXFLAGS=-shared -fPIC
LDFLAGS=-L$(OVIS_DIR)/lib

# Platform-specific libraries
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIBS=-lldmsd_stream -lldms
else
    LIBS=-lldmsd_stream -lldms -lrt
endif

all: kp_kernel_ldms.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))
CXXFLAGS+=-I${MAKEFILE_PATH}

kp_kernel_ldms.so: ${MAKEFILE_PATH}kp_kernel_ldms.cpp ${MAKEFILE_PATH}kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ ${MAKEFILE_PATH}kp_kernel_ldms.cpp \
	$(LIBS)

.PHONY: all clean

clean:
	$(RM) *.so
