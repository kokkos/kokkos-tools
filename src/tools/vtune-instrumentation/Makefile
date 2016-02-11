CXX=g++
CXXFLAGS=-O3 -std=c++11 -g
SHARED_CXXFLAGS=-shared -fPIC
VTUNE_PATH = /opt/intel/vtune_amplifier_xe_2015
all: kp_vtune_hooks.so

kp_vtune_hooks.so: kp_kernel_vtune.cpp kp_kernel_info_vtune.h
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) -o $@ kp_kernel_vtune.cpp -L${VTUNE_PATH}/lib64 -littnotify -lpthread -I${VTUNE_PATH}/include -I./ -I../simple-kernel-timer

clean:
	rm *.so
