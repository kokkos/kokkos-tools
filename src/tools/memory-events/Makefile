CXX=g++
CXXFLAGS=-O3 -std=c++11 -g -fopenmp
SHARED_CXXFLAGS=-shared -fPIC

all: kp_memory_events.so

kp_memory_events.so: kp_memory_events.cpp kp_memory_events.hpp kp_timer.hpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_memory_events.cpp

clean:
	rm *.so
