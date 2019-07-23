CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_systemtap_connector.so

probes.h: probes.d
	dtrace -C -h -s $< -o $@

probes.o: probes.d
	dtrace -C -G -s $< -o $@

kp_systemtap_connector.so: kp_systemtap_connector.cpp probes.h probes.o
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) probes.o \
		-o $@ kp_systemtap_connector.cpp $(LIBS)

clean:
	rm -rf *.so probes.h probes.o
