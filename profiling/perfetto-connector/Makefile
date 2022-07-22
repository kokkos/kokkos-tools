
OS = $(shell uname)
ifeq ($(OS), Linux)
	CXX := g++
	STRIPFLAGS := -w -K kokkosp_*
	CXXFLAGS_EXTRA :=
else
	CXX := clang++
	STRIPFLAGS := -x
	CXXFLAGS_EXTRA := -Wno-unused-const-variable
endif
CXXFLAGS := -O3 -std=c++17 -fPIC -fvisibility=hidden -fvisibility-inlines-hidden -W -Wall $(CXXFLAGS_EXTRA)

.PHONY: strip library clean

default: all

all: libperfetto-connector.so strip

libperfetto-connector.so: libperfetto-connector.cpp
	$(CXX) $(CXXFLAGS) -Iperfetto libperfetto-connector.cpp perfetto/perfetto.cc -shared -o libperfetto-connector.so

strip: libperfetto-connector.so
	strip $(STRIPFLAGS) libperfetto-connector.so

clean:
	rm -f libperfetto-connector.so

library: libperfetto-connector.so

.SILENT: clean
