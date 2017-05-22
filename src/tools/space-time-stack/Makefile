CXX=mpicxx
CFLAGS=-shared -O3 -fPIC -std=c++11 -Wall -Wextra

kp_space_time_stack.so: kp_space_time_stack.cpp
	$(CXX) $(CFLAGS) -o $@ $<

clean:
	rm *.so 
