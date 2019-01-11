CALIPER_HOME=/home/etphipp/project/tensors/genten/caliper/install/intel
LIBPFM_HOME=/home/software/libpfm/current
LIBUNWIND_HOME=/home/software/libunwind/current
CXX=icpc
CXXFLAGS=-O3 -std=c++11 -g -gxx-name=g++-4.9 -I$(CALIPER_HOME)/include
LDFLAGS=-L$(CALIPER_HOME)/lib64 -L$(LIBPFM_HOME)/lib -L$(LIBUNWIND_HOME)/lib
LIBS=-lcaliper -lcaliper-common -lcaliper-reader -lgotcha -lpfm -lunwind
SHARED_CXXFLAGS=-shared -fPIC

all: kp_caliper_connector.so

kp_caliper_connector.so: kp_caliper_connector.cpp
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) \
		-o $@ kp_caliper_connector.cpp $(LIBS)

clean:
	rm *.so
