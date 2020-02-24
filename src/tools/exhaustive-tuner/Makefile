CXX=clang++
CXXFLAGS=-O2 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC -I../../../include

all: exhaustive_tuner.so

exhaustive_tuner.so: exhaustive_tuner.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ exhaustive_tuner.cpp

clean:
	rm *.so
