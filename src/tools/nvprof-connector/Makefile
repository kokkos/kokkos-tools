CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(CUDA_ROOT)/include
LDFLAGS=-L$(CUDA_ROOT)/lib64
LIBS=-lnvToolsExt
SHARED_CXXFLAGS=-shared -fPIC

all: kp_nvtx_connector.so

kp_nvtx_connector.so: kp_nvtx_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ kp_nvtx_connector.cpp $(LIBS)

clean:
	rm *.so
