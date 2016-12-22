CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC

all: kp_kernel_timer.so

kp_kernel_timer.so: kp_kernel_timer.cpp kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_kernel_timer.cpp

clean:
	rm *.so
