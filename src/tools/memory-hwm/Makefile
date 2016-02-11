CXX=g++
CFLAGS=-shared -O3 -fPIC -std=c++11

kp_hwm.so: kp_hwm.cpp
	$(CXX) $(CFLAGS) -o $@ $<

clean:
	rm *.so 
