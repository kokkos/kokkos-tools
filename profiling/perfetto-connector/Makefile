default: all

all: libperfetto-connector.so

libperfetto-connector.so: libperfetto-connector.cc
	g++ -O3 -fPIC -I. libperfetto-connector.cc perfetto.cc -shared -o libperfetto-connector.so -std=c++17
