CXX=icpc
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_kernel_filter.so

kp_kernel_filter.so: kp_kernel_filter.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_kernel_filter.cpp

clean:
	rm *.so
