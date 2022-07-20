
CXX := g++
CXXFLAGS := -O3 -std=c++17 -fPIC -fvisibility=hidden -fvisibility-inlines-hidden -W -Wall

default: all

all: library strip

library: libperfetto-connector.cpp
	$(CXX) $(CXXFLAGS) -I. libperfetto-connector.cpp perfetto.cc -shared -o libperfetto-connector.so

strip: libperfetto-connector.so
	strip -w -K kokkosp_* libperfetto-connector.so

clean:
	rm -f libperfetto-connector.so

.SILENT: clean
