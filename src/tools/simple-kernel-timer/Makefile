CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC
# comment the following line if abi::__cxa_demangle is not supported by your compiler
CXXFLAGS+= -DHAVE_GCC_ABI_DEMANGLE

all: kp_kernel_timer.so kp_reader

kp_reader: kp_reader.cpp kp_kernel_timer.so
	$(CXX) $(CXXFLAGS) -o kp_reader kp_reader.cpp

kp_kernel_timer.so: kp_kernel_timer.cpp kp_kernel_info.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_kernel_timer.cpp

clean:
	rm *.so kp_reader
