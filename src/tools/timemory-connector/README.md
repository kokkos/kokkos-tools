# timemory KokkosP Profiling Tool

[timemory](https://github.com/NERSC/timemory) is a C++11 performance analysis library designed around a CRTP base class
and variadic template wrappers that enables arbitrary bundling of tools, measurements, and external APIs into
a single `start()` and `stop()` over the entire bundle.

- Timing
- Memory
- Resource Usage
- Hardware Counters
- [Roofline Performance Model](https://docs.nersc.gov/programming/performance-debugging-tools/roofline/)

## Relevant Links

- [GitHub](https://github.com/NERSC/timemory)
- [Documentation](https://timemory.readthedocs.io/en/latest/)
    - [Supported Components](https://timemory.readthedocs.io/en/latest/components/supported/)
- [Doxygen](https://timemory.readthedocs.io/en/latest/doxygen-docs/)

## timemory Install Requirements

- C++ compiler (gcc, clang, Intel)
- CMake v3.10 or higher
  - If you don't have a more recent CMake installation but have `conda`, this will provide a quick installation:
    - `conda create -n cmake -c conda-forge cmake`
    - `source activate cmake`
- [Installation Documentation](https://timemory.readthedocs.io/en/latest/installation/)


## Quick Start for Kokkos

### Installing timemory

In a separate location, clone timemory and install to a designated prefix. Here, `/opt/timemory` is the installation prefix:

```bash
git clone https://github.com/NERSC/timemory.git timemory
mkdir build-timemory && cd build-timemory
cmake -DTIMEMORY_BUILD_C=OFF -DCMAKE_INSTALL_PREFIX=/opt/timemory ../timemory
make install -j8
```

### Building timemory-connector

From the folder of this document, it is recommended to build with CMake but a Makefile is provided:

```bash
mkdir build && cd build
cmake -Dtimemory_DIR=/opt/timemory ..
make
```

## Run kokkos application with timemory enabled

Before executing the Kokkos application you have to set the environment variable `KOKKOS_PROFILE_LIBRARY` to point to the name of the dynamic library. Also add the library path of PAPI and PAPI connector to `LD_LIBRARY_PATH`.

```console
export KOKKOS_PROFILE_LIBRARY=kp_timemory.so
export LD_LIBRARY_PATH=<TIMEMORY-LIB-PATH>:<TIMEMORY-CONNECTOR-LIB-PATH>:$LD_LIBRARY_PATH
```

timemory can enable measurments of several different metrics ("components") simulatenously.
Configure the `TIMEMORY_COMPONENTS` environment variable to set an exact specification of the desired components.

```console
export TIMEMORY_COMPONENTS="wall_clock,peak_rss"
```

Refer to the [supported components documentation](https://timemory.readthedocs.io/en/latest/components/supported/)
for the full list of components. By default `TIMEMORY_COMPONENTS` records:

| Metric         | Description                                              |
| -------------- | -------------------------------------------------------- |
| `wall_clock`   | Wall-clock runtime                                       |
| `system_clock` | CPU timer spent in kernel-mode                           |
| `user_clock`   | CPU time spent in user-mode                              |
| `cpu_util`     | CPU utilization                                          |
| `page_rss`     | High-water (allocated) memory mark in region             |
| `peak_rss`     | High-water (used) memory mark in region (excluding swap) |

### Appending components

Other components in addition to those specified by `TIMEMORY_COMPONENTS`
can be enabled at runtime via the `TIMEMORY_COMPONENT_LIST_INIT` environment variable.
These components may include:

| Metric                  | Description                                                                        |
| ----------------------- | ---------------------------------------------------------------------------------- |
| `caliper`               | Enable [Caliper toolkit](https://github.com/LLNL/Caliper)                          |
| `papi_array`            | Enable PAPI hardware counters                                                      |
| `cpu_roofline_sp_flops` | Enable single-precision floating-point roofline on CPU                             |
| `cpu_roofline_dp_flops` | Enable single-precision floating-point roofline on CPU                             |
| `gperf_cpu_profiler`    | Enable gperftools CPU profiler (sampler)                                           |
| `gperf_heap_profiler`   | Enable gperftools heap profiler                                                    |
| `cuda_event`            | Enable `cudaEvent_t` for approximate kernel runtimes                               |
| `nvtx_marker`           | Enable NVTX markers                                                                |
| `cupti_activity`        | Enable CUPTI runtime tracing on NVIDIA GPUs                                        |
| `cupti_counters`        | Enable CUPTI hardware counters on NVIDIA GPUs                                      |
| `gpu_roofline_flops`    | Enable half-, single-, and double-precision floating-point roofline on NVIDIA GPUs |
| `gpu_roofline_hp_flops` | Enable half-precision floating-point roofline on NVIDIA GPUs                       |
| `gpu_roofline_sp_flops` | Enable single-precision floating-point roofline on NVIDIA GPUs                     |
| `gpu_roofline_dp_flops` | Enable double-precision floating-point roofline on NVIDIA GPUs                     |

Some components will conflict with each other and should not be enabled simulatenously. Such combinations include:

- `papi_array` and `cpu_roofline` types
- `cupti_activity` and `cupti_counters`
- `cupti_activity`/`cupti_counters` and `gpu_roofline` types
- Any hardware counters + `caliper` (when Caliper is also configured to collect hardware counters)

```console
export TIMEMORY_COMPONENT_LIST_INIT="cupti_activity,papi_array"
```

## Run kokkos application with PAPI recording enabled

When timemory was installed with PAPI support, assuming `CMAKE_INSTALL_RPATH_USE_LINK_PATH=ON`, the timemory
library will know the location of the PAPI library. However, if this is not the case, add the path to the PAPI
library to `LD_LIBRARY_PATH`.

Internally, timemory uses the `TIMEMORY_PAPI_EVENTS` environment variable for specifying arbitrary events.
However, this library will attempt to read `PAPI_EVENTS` and set `TIMEMORY_PAPI_EVENTS` before the PAPI
component is initialized, if using `PAPI_EVENTS` does not provide the desired events, use `TIMEMORY_PAPI_EVENTS`.

Example enabling (1) total instructions, (2) total cycles, (3) total load/stores

```console
export PAPI_EVENTS="PAPI_TOT_INS,PAPI_TOT_CYC,PAPI_LST_INS"
export TIMEMORY_PAPI_EVENTS="PAPI_TOT_INS,PAPI_TOT_CYC,PAPI_LST_INS"
```

## Run kokkos application with Roofline recording enabled

[Roofline Performance Model](https://docs.nersc.gov/programming/performance-debugging-tools/roofline/)

On both the CPU and GPU, calculating the roofline requires two passes.
It is recommended to use the timemory python interface to generate the roofline because
the `timemory.roofline` submodule provides a mode that will handle executing the application
twice and generating the plot. For advanced usage, see the
[timemory Roofline Documentation](https://timemory.readthedocs.io/en/latest/getting_started/roofline/).


1. Set environment variable `KOKKOS_ROOFLINE=ON`
    - This configures output to __*always*__ output to `${TIMEMORY_OUTPUT_PATH}` (default: `timemory-output/`)
2. Set environment variable `TIMEMORY_OUTPUT_PATH` as desired
3. Execute application with `python -m timemory.roofline <OPTIONS> -- <COMMAND>`
    - `OPTIONS` are the roofline module options
        - `-d` : display plot immediately
        - `-k` : ignore exit code of applicaiton
        - `-n` : number of threads
            - *IMPORTANT*: this will set the appropriate timemory environment variable to ensure the empirical peak
              for the hardware is calculated properly
        - `-f` : image format
        - `-t` : type of the roofline to plot
            - CPU and GPU rooflines can both be enabled but only one plot will be generated
    - `COMMAND` is the command and arguments for the application
        - `python -m timemory.roofline --help` : provides help for arguments when output is already generated
        - `python -m timemory.roofline --help -- <COMMAND>` : provides help for executing an application

```console
export KOKKOS_ROOFLINE=ON
export TIMEMORY_OUTPUT_PATH=${HOSTNAME}_roofline
export OMP_NUM_THREADS=4
export TIMEMORY_COMPONENT_LIST_INIT="cpu_roofline_dp_flops"
python -m timemory.roofline -n 4 -f png -t cpu_roofline -- ./sample
```

## Included Multithreaded Sample

### Building Sample

```shell
cmake -DBUILD_SAMPLE=ON ..
make -j2
```

### Running Sample

#### Command

```shell
$ export TIMEMORY_JSON_OUTPUT=ON
$ export TIMEMORY_COMPONENTS="wall_clock,peak_rss,page_rss,thread_cpu_clock,thread_cpu_util,cpu_clock,cpu_util"
$ export LD_PRELOAD=${PWD}/kp_timemory.so
$ time ./sample
```

#### Notes

- `[0] fibonacci_8` and all subsequent measurements are scoped as such because
  `[0] thread_creation` completed the thread creation and ended before those threads were launched
- `[0]` designates the device number if provided to connector (parallel_for/parallel_reduce/parallel_scan)
- `time` is used to demonstrate that the top-level `real_clock`/`wall_clock` timer produces a correct execution time
  for the entire application.

#### Output

```shell
#-------------------------------------------------------------------------#
KokkosP: TiMemory Connector (sequence is 0, version: 0)
#-------------------------------------------------------------------------#

fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073
fibonacci(47) = 2971215073

#-------------------------------------------------------------------------#
KokkosP: Finalization of TiMemory Connector. Complete.
#-------------------------------------------------------------------------#


[thread_cpu_util]> Outputting 'docker-desktop_41392/thread_cpu_util.txt'... Done
[thread_cpu_util]> Outputting 'docker-desktop_41392/thread_cpu_util.json'... Done

> [cxx] sample                      :    0.0 % thread_cpu_util, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :   46.9 % thread_cpu_util, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_0         :   98.5 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.4 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         :   97.9 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.7 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         :   95.5 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.7 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         :   96.3 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.7 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         :   97.0 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.6 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         :   94.2 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.2 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         :   93.0 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.5 % thread_cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         :   93.4 % thread_cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :   99.2 % thread_cpu_util, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           :   92.1 % thread_cpu_util, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_runtime   :   99.5 % thread_cpu_util, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           :   91.6 % thread_cpu_util, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_runtime   :   99.8 % thread_cpu_util, 1 laps, depth 2

[thread_cpu]> Outputting 'docker-desktop_41392/thread_cpu.txt'... Done
[thread_cpu]> Outputting 'docker-desktop_41392/thread_cpu.json'... Done

> [cxx] sample                      :    0.003 sec thread_cpu, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :    0.002 sec thread_cpu, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_0         :   11.124 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.0%)
> [cxx]     |_[0] fibonacci_runtime :   11.121 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         :   11.059 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.0%)
> [cxx]     |_[0] fibonacci_runtime :   11.056 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         :   11.395 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.1%)
> [cxx]     |_[0] fibonacci_runtime :   11.388 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         :   11.306 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.0%)
> [cxx]     |_[0] fibonacci_runtime :   11.300 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         :   11.243 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.0%)
> [cxx]     |_[0] fibonacci_runtime :   11.239 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         :   11.170 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.1%)
> [cxx]     |_[0] fibonacci_runtime :   11.162 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         :   11.354 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.1%)
> [cxx]     |_[0] fibonacci_runtime :   11.344 sec thread_cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         :   11.225 sec thread_cpu, 1 laps, depth 2 (exclusive:   0.1%)
> [cxx]     |_[0] fibonacci_runtime :   11.213 sec thread_cpu, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           :   11.277 sec thread_cpu, 1 laps, depth 1 (exclusive:   0.1%)
> [cxx]   |_[0] fibonacci_runtime   :   11.264 sec thread_cpu, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           :   11.245 sec thread_cpu, 1 laps, depth 1 (exclusive:   0.1%)
> [cxx]   |_[0] fibonacci_runtime   :   11.229 sec thread_cpu, 1 laps, depth 2

[real]> Outputting 'docker-desktop_41392/real.txt'... Done
[real]> Outputting 'docker-desktop_41392/real.json'... Done

> [cxx] sample                      :   13.280 sec real, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :    0.004 sec real, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_0         :   11.296 sec real, 1 laps, depth 2 (exclusive:   0.9%)
> [cxx]     |_[0] fibonacci_runtime :   11.193 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         :   11.298 sec real, 1 laps, depth 2 (exclusive:   1.8%)
> [cxx]     |_[0] fibonacci_runtime :   11.095 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         :   11.934 sec real, 1 laps, depth 2 (exclusive:   4.3%)
> [cxx]     |_[0] fibonacci_runtime :   11.427 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         :   11.746 sec real, 1 laps, depth 2 (exclusive:   3.5%)
> [cxx]     |_[0] fibonacci_runtime :   11.339 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         :   11.591 sec real, 1 laps, depth 2 (exclusive:   2.6%)
> [cxx]     |_[0] fibonacci_runtime :   11.286 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         :   11.861 sec real, 1 laps, depth 2 (exclusive:   5.1%)
> [cxx]     |_[0] fibonacci_runtime :   11.252 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         :   12.215 sec real, 1 laps, depth 2 (exclusive:   6.6%)
> [cxx]     |_[0] fibonacci_runtime :   11.404 sec real, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         :   12.014 sec real, 1 laps, depth 2 (exclusive:   5.9%)
> [cxx]     |_[0] fibonacci_runtime :   11.302 sec real, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           :   12.238 sec real, 1 laps, depth 1 (exclusive:   7.5%)
> [cxx]   |_[0] fibonacci_runtime   :   11.323 sec real, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           :   12.273 sec real, 1 laps, depth 1 (exclusive:   8.3%)
> [cxx]   |_[0] fibonacci_runtime   :   11.256 sec real, 1 laps, depth 2

[peak_rss]> Outputting 'docker-desktop_41392/peak_rss.txt'... Done
[peak_rss]> Outputting 'docker-desktop_41392/peak_rss.json'... Done

> [cxx] sample                      : 216.3 MB peak_rss, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :   1.9 MB peak_rss, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_0         : 215.7 MB peak_rss, 1 laps, depth 2 (exclusive:   2.6%)
> [cxx]     |_[0] fibonacci_runtime : 210.1 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         : 216.0 MB peak_rss, 1 laps, depth 2 (exclusive:   6.1%)
> [cxx]     |_[0] fibonacci_runtime : 202.8 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         : 214.4 MB peak_rss, 1 laps, depth 2 (exclusive:  27.1%)
> [cxx]     |_[0] fibonacci_runtime : 156.4 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         : 214.4 MB peak_rss, 1 laps, depth 2 (exclusive:  18.0%)
> [cxx]     |_[0] fibonacci_runtime : 175.9 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         : 214.3 MB peak_rss, 1 laps, depth 2 (exclusive:  10.7%)
> [cxx]     |_[0] fibonacci_runtime : 191.3 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         : 214.4 MB peak_rss, 1 laps, depth 2 (exclusive:  37.9%)
> [cxx]     |_[0] fibonacci_runtime : 133.1 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         : 214.4 MB peak_rss, 1 laps, depth 2 (exclusive:  65.1%)
> [cxx]     |_[0] fibonacci_runtime :  74.8 MB peak_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         : 214.4 MB peak_rss, 1 laps, depth 2 (exclusive:  50.6%)
> [cxx]     |_[0] fibonacci_runtime : 106.0 MB peak_rss, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           : 214.4 MB peak_rss, 1 laps, depth 1 (exclusive:  81.5%)
> [cxx]   |_[0] fibonacci_runtime   :  39.7 MB peak_rss, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           : 214.4 MB peak_rss, 1 laps, depth 1 (exclusive:  99.7%)
> [cxx]   |_[0] fibonacci_runtime   :   0.6 MB peak_rss, 1 laps, depth 2

[page_rss]> Outputting 'docker-desktop_41392/page_rss.txt'... Done
[page_rss]> Outputting 'docker-desktop_41392/page_rss.json'... Done

> [cxx] sample                      : 174.2 MB page_rss, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :   1.9 MB page_rss, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_0         : 220.8 MB page_rss, 1 laps, depth 2 (exclusive:   2.6%)
> [cxx]     |_[0] fibonacci_runtime : 215.2 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         : 221.1 MB page_rss, 1 laps, depth 2 (exclusive:   6.1%)
> [cxx]     |_[0] fibonacci_runtime : 207.6 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         : 205.8 MB page_rss, 1 laps, depth 2 (exclusive:  29.8%)
> [cxx]     |_[0] fibonacci_runtime : 144.5 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         : 219.6 MB page_rss, 1 laps, depth 2 (exclusive:  18.0%)
> [cxx]     |_[0] fibonacci_runtime : 180.1 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         : 219.4 MB page_rss, 1 laps, depth 2 (exclusive:  10.7%)
> [cxx]     |_[0] fibonacci_runtime : 195.9 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         : 203.7 MB page_rss, 1 laps, depth 2 (exclusive:  40.9%)
> [cxx]     |_[0] fibonacci_runtime : 120.4 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         : 204.2 MB page_rss, 1 laps, depth 2 (exclusive:  70.0%)
> [cxx]     |_[0] fibonacci_runtime :  61.2 MB page_rss, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         : 204.0 MB page_rss, 1 laps, depth 2 (exclusive:  54.5%)
> [cxx]     |_[0] fibonacci_runtime :  92.9 MB page_rss, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           : 172.3 MB page_rss, 1 laps, depth 1 (exclusive: 100.0%)
> [cxx]   |_[0] fibonacci_runtime   :   0.0 MB page_rss, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           : 172.3 MB page_rss, 1 laps, depth 1 (exclusive: 100.0%)
> [cxx]   |_[0] fibonacci_runtime   :   0.0 MB page_rss, 1 laps, depth 2

[cpu_util]> Outputting 'docker-desktop_41392/cpu_util.txt'... Done
[cpu_util]> Outputting 'docker-desktop_41392/cpu_util.json'... Done

> [cxx] sample                      :  846.6 % cpu_util, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :    0.0 % cpu_util, 1 laps, depth 1 (exclusive:   0.0%)
> [cxx]   |_[0] fibonacci_0         :  946.7 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  955.3 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         :  947.0 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  963.5 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         :  933.2 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  966.0 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         :  939.2 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  967.5 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         :  942.6 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  965.5 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         :  936.0 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  973.3 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         :  919.3 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  960.0 % cpu_util, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         :  929.6 % cpu_util, 1 laps, depth 2
> [cxx]     |_[0] fibonacci_runtime :  969.5 % cpu_util, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           :  918.2 % cpu_util, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_runtime   :  960.5 % cpu_util, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           :  915.9 % cpu_util, 1 laps, depth 1
> [cxx]   |_[0] fibonacci_runtime   :  957.7 % cpu_util, 1 laps, depth 2

[cpu]> Outputting 'docker-desktop_41392/cpu.txt'... Done
[cpu]> Outputting 'docker-desktop_41392/cpu.json'... Done

> [cxx] sample                      :  112.420 sec cpu, 1 laps, depth 0
> [cxx] |_[0] thread_creation       :    0.000 sec cpu, 1 laps, depth 1 (exclusive:   0.0%)
> [cxx]   |_[0] fibonacci_0         :  106.930 sec cpu, 1 laps, depth 2 (exclusive:   0.0%)
> [cxx]     |_[0] fibonacci_runtime :  106.930 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_1         :  107.000 sec cpu, 1 laps, depth 2 (exclusive:   0.1%)
> [cxx]     |_[0] fibonacci_runtime :  106.900 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_4         :  111.370 sec cpu, 1 laps, depth 2 (exclusive:   0.9%)
> [cxx]     |_[0] fibonacci_runtime :  110.380 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_3         :  110.310 sec cpu, 1 laps, depth 2 (exclusive:   0.5%)
> [cxx]     |_[0] fibonacci_runtime :  109.710 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_2         :  109.260 sec cpu, 1 laps, depth 2 (exclusive:   0.3%)
> [cxx]     |_[0] fibonacci_runtime :  108.960 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_5         :  111.020 sec cpu, 1 laps, depth 2 (exclusive:   1.4%)
> [cxx]     |_[0] fibonacci_runtime :  109.510 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_7         :  112.300 sec cpu, 1 laps, depth 2 (exclusive:   2.5%)
> [cxx]     |_[0] fibonacci_runtime :  109.480 sec cpu, 1 laps, depth 3
> [cxx]   |_[0] fibonacci_6         :  111.690 sec cpu, 1 laps, depth 2 (exclusive:   1.9%)
> [cxx]     |_[0] fibonacci_runtime :  109.570 sec cpu, 1 laps, depth 3
> [cxx] |_[0] fibonacci_8           :  112.370 sec cpu, 1 laps, depth 1 (exclusive:   3.2%)
> [cxx]   |_[0] fibonacci_runtime   :  108.760 sec cpu, 1 laps, depth 2
> [cxx] |_[0] fibonacci_9           :  112.410 sec cpu, 1 laps, depth 1 (exclusive:   4.1%)
> [cxx]   |_[0] fibonacci_runtime   :  107.800 sec cpu, 1 laps, depth 2

real	0m13.748s
user	1m52.270s
sys	    0m0.190s
```
