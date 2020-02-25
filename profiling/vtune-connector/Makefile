CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(VTUNE_HOME)/include
LDFLAGS=-L$(VTUNE_HOME)/lib64
LIBS=-littnotify
SHARED_CXXFLAGS=-shared -fPIC

all: kp_vtune_connector.so

kp_vtune_connector.so: kp_vtune_connector.cpp kp_vtune_connector_domain.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ kp_vtune_connector.cpp $(LIBS)

clean:
	rm *.so
