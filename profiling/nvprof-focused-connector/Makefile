CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -I$(CUDA_ROOT)/include -I./
LDFLAGS=-L$(CUDA_ROOT)/lib64
LIBS=-lnvToolsExt -lcudart
SHARED_CXXFLAGS=-shared -fPIC

all: kp_nvprof_focused_connector.so

kp_nvprof_focused_connector.so: kp_nvprof_focused_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ kp_nvprof_focused_connector.cpp $(LIBS)

clean:
	rm *.so
