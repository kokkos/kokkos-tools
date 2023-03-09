

# Kokkos Tools

Kokkos Tools provide a light-weight set of profiling and debugging utilities, which interface with instrumentation hooks built directly into the Kokkos runtime. Compared to 3rd party tools these tools can provide much cleaner, context-specific information: in particular, they allow kernel-centric analysis and they use labels provided to Kokkos constructs (kernel launches and views).

Under most circumstances, the profiling hooks are compiled into Kokkos executables by default assuming that the profiling hooks' version is compatible with the tools' version. No recompilation or changes to your build procedures are required.

Note: `Kokkos` must be configured with `Kokkos_ENABLE_LIBDL=ON` to load profiling hooks dynamically. This is the default for most cases anyway.

## General Usage

 To use one of the tools you have to compile it, which will generate a dynamic library. Before executing the Kokkos application you then have to set the environment variable `KOKKOS_TOOLS_LIB` to point to the dynamic library e.g. in Bash:
```
export KOKKOS_TOOLS_LIB=${HOME}/kokkos-tools/src/tools/memory-events/kp_memory_event.so
```

Many of the tools will produce an output file that uses the hostname as well as the process id as part of the filename. 

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

### Utilities

+ **[[KernelFilter|KernelFilter]]:**

   A tool that is used in conjunction with analysis tools, to restrict them to a subset of the application.

### Memory Analysis
+ **[[MemoryHighWater|MemoryHighWater]]:**

    Outputs high water mark of memory usage of the application.

+ **[[MemoryUsage|MemoryUsage]]:**

    Generates a per Memory Space timeline of memory utilization. 

+ **[[MemoryEvents|MemoryEvents]]:** 

    Tool to track memory events such as allocation and deallocation. It also provides the information of the MemoryUsage tool.

### Kernel Inspection
+ **[[SimpleKernelTimer|SimpleKernelTimer]]:**

    Captures basic timing information for Kernels.

+ **[[KernelLogger|KernelLogger]]:**

    Prints kernel and region events during runtime.

### 3rd Party Profiling Tool Hooks
+ **[[VTuneConnector|VTuneConnector]]:**
    
    Provides Kernel Names to VTune, so that analysis can be performed on a per kernel base.

+ **[[VTuneFocusedConnector|VTuneFocusedConnector]]**
    
    Like VTuneConnector but turns profiling off outside of kernels. Should be used in conjunction with the KernelFilter tool. 

+ **[[Timemory|Timemory]]:**

    Modular connector for accumulating timing, memory usage, hardware counters, and other various metrics.
    Supports controlling VTune, CUDA profilers, and TAU + kernel name forwarding to VTune, NVTX, TAU,
    Caliper, and LIKWID.

    #####  If you need to write your own plug-in, this provides a straight-forward API to writing the plug-in.

    Defining a timemory component will enable your plug-in to output to stdout, text, and JSON, 
    accumulate statistics, and utilize various portable function calls for common needs w.r.t. timers,
    resource usage, etc. 


# Building Kokkos Tools

Use either CMake or provided Makefile within each subdirectory of Kokkos Tools.

Building with Makefiles is currently recommended. 

# Running a Kokkos-based Application with a tool

Given your tool shared library <name_of_tool_shared_library>.so (which contains kokkos profiling callback functions) and an application executable called yourApplication.exe, type: 

`export KOKKOS_TOOLS_LIB=${YOUR_KOKKOS_TOOLS_DIR}/<name_of_tool_shared_lib>; ./yourApplication.exe 

# Documentation

Further information and tutorials can be found here: https://github.com/kokkos/kokkos-tutorials/blob/main/LectureSeries/KokkosTutorial_07_Tools.pdf


# Contact 

* Vivek Kale (vlkale@sandia.gov)
* Christian Trott (crtrott@sandia.gov)

# Acknowledgement

Special thanks to David Poliakoff on earlier work on tools development.
