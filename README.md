# KokkosP Profiling Tools 

## What 
Performance profiling tools that can be optionally integrated into Kokkos-based applications and libraries.


## Why 
- Generally, understanding performance and correctness of Kokkos programs in a performance portable way. How do we do apples-to-apples comparison for detailed profiling? 
- Name demangling from backend tools output
- Uniform interface for backend tools, e.g., NVprof, Rocprof

## Examples of existing tools: 

### Performance Profiling
- SimpleKernelTimer 
- SpaceTimeStack

### Debugging
- kp_reader
- kp_kernel_logger
- kp_kernel_filter

# Using this Software 
## Build Kokkos tools 

To build all the tools, simply create a build directory and type `cmake ..` in it. You can configure the cmake options through `ccmake ..`.  Doing so is not recommended. The build has been tested sucessfully on Perlmutter and MacOS.

## Making Kokkos Application code invoke Kokkos tools

Use Kokkos pushRegion and popRegion surrounding regions that you want to profile on. 

## Running a Kokkos-based Application with a tool 

Given your tool _<name_of_tool>_ (which contains kokkos profiling callback functions) and an application executable called yourApplication.exe, type 
export KOKKOS_TOOL_LIB=${YOUR_KOKKOS_TOOLS_DIR}/libkp_<name_of_tool>.dylib; ./yourApplication.exe


# Notes for Developers
- Kokkos callback functions, e.g., kokkosp_callback_fence(..) are inserted through binary instrumentation of the application. 
- Developing your own connector is possible. 
- The new cmake build process is being tested on various machines in the DoE. Avoid building with systemtap-connector when using cmake. 
- An alternate 'make' build process is still available in the master branch. 
- Each Kokkos tool is dynamic libraries when building via cmake (dylib) but a shared object (.so) file when using a Makefile to build. 


# Documentation

https://github.com/kokkos/kokkos-tutorials/blob/main/LectureSeries/KokkosTutorial_07_Tools.pdf


# Contact 

* Vivek Kale (vlkale@sandia.gov)
* Christian Trott (crtrott@sandia.gov) 

# Acknowledgement

Special thanks to David Poliakoff on earlier work on tools development.
