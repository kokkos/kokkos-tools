CXX = mpicxx
CXXFLAGS = -shared -O3 -fPIC -std=c++11

kp_hwm_mpi.so: kp_hwm_mpi.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm *.so 
