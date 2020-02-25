CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -fopenmp
SHARED_CXXFLAGS=-shared -fPIC

all: kp_memory_usage.so

kp_memory_usage.so: kp_memory_usage.cpp kp_memory_events.hpp kp_timer.hpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_memory_usage.cpp

clean:
	rm *.so
