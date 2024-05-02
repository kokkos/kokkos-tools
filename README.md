# Kokkos Tools

Kokkos Tools provide a set of light-weight of profiling and debugging utilities, which interface with instrumentation hooks built directly into the Kokkos runtime. Compared to 3rd party tools these tools can provide much cleaner, context-specific information: in particular, they allow kernel-centric analysis and they use labels provided to Kokkos constructs (kernel launches and views).

Under most circumstances, the profiling hooks are compiled into Kokkos executables by default assuming that the profiling hooks' version is compatible with the tools' version. No recompilation or changes to your build procedures are required.

Note: `Kokkos` must be configured with `Kokkos_ENABLE_LIBDL=ON` to load profiling hooks dynamically. This is the default for most cases anyway.

## General Usage

To use one of the tools you have to compile it, which will generate a dynamic library. Before executing the Kokkos application you then have to set the environment variable `KOKKOS_TOOLS_LIBS` to point to the dynamic library.

CMake and Makefiles are supported for building Kokkos Tools. The following provides instructions for both.

## Using cmake 

### Build

1. Create a build directory in Kokkos Tools, e.g., type `mkdir myBuild; cd myBuild`
2. To configure, type `cmake .. -DCMAKE_INSTALL_PREFIX=${YOUR_KOKKOS_TOOLS_INSTALL_DIR}`. There are more options but in most cases the defaults are sufficient.
3. To compile, type `make`
4. To install, type `make install`

### Run 

Given your installed tool shared library `lib<name_of_tool_shared_lib>.so` and an application executable called yourApplication.exe, type: 

`export KOKKOS_TOOLS_LIBS=${YOUR_KOKKOS_TOOLS_INSTALL_DIR}/lib<name_of_tool_shared_lib>.so; ./yourApplication.exe`


## Using make

### Build

To build some library `<name_of_tool_shared_lib>` with make, simply type `make` within that library's subdirectory `${YOUR_KOKKOS_TOOLS_LIB_SRC_DIR}` of Kokkos Tools. This generates the shared library within that subdirectory. 

### Run 

Given your installed tool shared library `<name_of_tool_shared_lib>.so` and an application executable called `yourApplication.exe`, type: 

`export KOKKOS_TOOLS_LIBS=${YOUR_KOKKOS_TOOLS_LIB_SRC_DIR}/<name_of_tool_shared_lib>.so; ./yourApplication.exe`


## Explicit Instrumentation

One can explicitly add instrumentation to a library or an application. Currently, the only hooks intended for explicit programmer use are the Region related hooks. These use a push/pop model to mark coarser regions in your code.

```c++
void foo() {
   Kokkos::Profiling::pushRegion("foo");
   bar();
   stool();
   Kokkos::Profiling::popRegion();
}
```

## Tools

The following provides an overview of the tools available in the set of Kokkos Tools. Click on each Kokkos Tools name to see more details about the tool via the Kokkos Tools Wiki. 

### Utilities
+ [**KernelFilter:**](https://github.com/kokkos/kokkos-tools/wiki/KernelFilter) 

    A tool which is used in conjunction with analysis tools, to restrict them to a subset of the application.

+ [**KernelSampler:**](https://github.com/kokkos/kokkos-tools/wiki/KernelSampler)

   A tool to be used in conjunction with analysis tools to restrict the tooling to samples of Kokkos kernel invocations.

### Memory Analysis
+ [**MemoryHighWater:**](https://github.com/kokkos/kokkos-tools/wiki/MemoryHighWater)

    This tool outputs the _high water mark_ of memory usage of the application. The _high water mark_ of memory usage is the highest amount of memory that is being utilized during the application's execution. 

+ [**MemoryUsage:**](https://github.com/kokkos/kokkos-tools/wiki/MemoryUsage)

    Generates a per Memory Space timeline of memory utilization. 

+ [**MemoryEvents:**](https://github.com/kokkos/kokkos-tools/wiki/MemoryEvents)

    Tool to track memory events such as allocation and deallocation. It also provides the information of the MemoryUsage tool.

### Kernel Inspection
+ [**SimpleKernelTimer**](https://github.com/kokkos/kokkos-tools/wiki/SimpleKernelTimer)

    Captures basic timing information for Kernels.

+ [**KernelLogger**](https://github.com/kokkos/kokkos-tools/wiki/KernelLogger)

    Prints Kokkos Kernel and Region events during runtime.

### 3rd Party Profiling Tool Hooks
+ [**VTuneConnector:**](https://github.com/kokkos/kokkos-tools/wiki/VTuneConnector)
    
    Provides Kokkos Kernel Names to VTune, so that analysis can be performed on a per kernel base.

+ [**VTuneFocusedConnector:**](https://github.com/kokkos/kokkos-tools/wiki/VTuneFocusedConnector)
    
    Like VTuneConnector but turns profiling off outside of kernels. Should be used in conjunction with the KernelFilter tool. 

+ [**NVTXConnector:**](https://github.com/kokkos/kokkos-tools/wiki/NVTXConnector)

    Provides Kokkos Kernel Names to NVTX, so that analysis can be performed on a per kernel base.

+ [**Timemory:**](https://github.com/kokkos/kokkos-tools/wiki/Timemory)

    Modular connector for accumulating timing, memory usage, hardware counters, and other various metrics.
    Supports controlling VTune, CUDA profilers, and TAU + kernel name forwarding to VTune, NVTX, TAU,
    Caliper, and LIKWID.

    ##### If you need to write your own plug-in, this provides a straight-forward API to writing the plug-in.

    Defining a timemory component will enable your plug-in to output to stdout, text, and JSON, 
    accumulate statistics, and utilize various portable function calls for common needs w.r.t. timers,
    resource usage, etc. 

# Tutorial

A tutorial on Kokkos Tools can be found here: https://github.com/kokkos/kokkos-tutorials/blob/main/LectureSeries/KokkosTutorial_07_Tools.pdf

# Contact 

* Vivek Kale (vlkale@sandia.gov)
* Christian Trott (crtrott@sandia.gov)

# Acknowledgement

Special thanks to David Poliakoff on earlier work on Kokkos Tools development.
