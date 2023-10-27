# How to Build 

# With Cmake  

1. Create your build directory and go to it

2. Type `ccmake ..`  and change any options, including tools you want turned on  (some are by default off).  (Optional)

3. Type `cmake ..` 

4. Type `make` 

5. Specify the generated .dylib file in the environment variable KOKKOS_TOOLS_LIBRARY when running your Kokkos-based application. 


# With Makefile (recommended)

1. Go into the directory of the particular tool, e.g., `cd debugging/kernel_logger`

2. Type `make` 

3. Specify the generated .so file in the environment variable KOKKOS_TOOLS_LIBRARY when running your Kokkos-based application. 
