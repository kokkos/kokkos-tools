CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(PAPI_ROOT)/include
LDFLAGS=-L$(PAPI_ROOT)/lib
LIBS=-lpapi
SHARED_CXXFLAGS=-shared -fPIC

all: kp_papi_connector.so

kp_papi_connector.so: kp_papi_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ kp_papi_connector.cpp $(LIBS)

clean:
	rm *.so
