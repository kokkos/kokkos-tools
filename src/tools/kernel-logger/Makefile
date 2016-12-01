CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_kernel_logger.so

kp_kernel_logger.so: kp_kernel_logger.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_kernel_logger.cpp

clean:
	rm *.so
