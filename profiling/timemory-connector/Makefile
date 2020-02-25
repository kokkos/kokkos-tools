TIMEMORY_ROOT_DIR:=/usr/local
CXX=nvcc
CXXFLAGS=-O3 -std=c++11 -g -I$(TIMEMORY_ROOT_DIR)/include
LDFLAGS=-L$(TIMEMORY_ROOT_DIR)/lib -L$(TIMEMORY_ROOT_DIR)/lib64 -ltimemory
SHARED_CXXFLAGS=-shared --compiler-options=-fPIC -x cu

all: kp_timemory.so

kp_timemory.so: kp_timemory.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_timemory.cpp $(LDFLAGS)

format:
	clang-format -i *.hpp *.cpp

clean:
	rm -f *.so
