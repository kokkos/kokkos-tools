# Power Profiler: Energy Consumption Toolbox using Variorum

This Kokkos tool is designed to monitor and log GPU power consumption and kernel execution times for applications built with Kokkos. It leverages the [Variorum](https://variorum.readthedocs.io/en/latest/) library to interface with hardware and collect power data.

## Features

* **GPU Power Monitoring**: Continuously samples and logs power consumption from available GPUs using Variorum.
* **Kernel Timing**: Records the start and end times of Kokkos `parallel_for`, `parallel_scan`, and `parallel_reduce` kernels.
* **Profiling Regions**: Allows for custom profiling regions to be defined within your Kokkos application, helping to categorize and analyze energy usage for specific code sections.
* **Detailed Output**: Provides structured output at finalization, detailing both power readings and kernel timings, suitable for post-processing and analysis.
* **Multi-threading Support**: Utilizes a dedicated background thread for power monitoring to minimize overhead on the main application execution. Thread-safe data logging is ensured using mutexes.

## Specifications

This profiler integrates with Kokkos via its profiling interface (`KokkosP`).

1.  **Initialization (`kokkosp_init_library`)**:
    * Scans for available GPU devices using Variorum to identify which GPUs can be monitored.
    * Starts a dedicated `std::jthread` (C++20) to continuously collect GPU power readings at a defined interval (currently 20ms).
2.  **Kernel Hooking**:
    * Hooks into `parallel_for`, `parallel_scan`, and `parallel_reduce` kernel events. When a kernel begins, its name, type, and start timestamp are recorded. When it ends, the end timestamp is recorded, and the complete timing information is added to a log.
4.  **Finalization (`kokkosp_finalize_library`)**:
    * Stops the background power monitoring thread.
    * Prints all collected power readings and kernel timings to standard output in a machine-readable format, delimited by `--- POWER_DATA_START ---`, `--- POWER_DATA_END ---`, `--- KERNEL_DATA_START ---`, and `--- KERNEL_DATA_END ---`.

### Output Format

#### Power Data

Each line under `--- POWER_DATA_START ---` follows this format:

`POWER_READING,<timestamp_ms>,<gpu_id>,<power_watts>`

* `<timestamp_ms>`: Epoch timestamp in milliseconds.
* `<gpu_id>`: The ID of the GPU device (e.g., 0, 1, etc.).
* `<power_watts>`: The power consumption in Watts for that GPU at the given timestamp.

Example:

```
POWER_READING,1678886400000,0,50.5
POWER_READING,1678886400020,0,52.1
```

#### Kernel Data

Each line under `--- KERNEL_DATA_START ---` follows this format:

`KERNEL_TIMING,<kernel_type>,"<kernel_name>",<start_time_ms>,<end_time_ms>,<duration_ms>`

* `<kernel_type>`: Type of the Kokkos kernel (e.g., `For`, `Scan`, `Reduce`).
* `<kernel_name>`: The name of the kernel as provided in the Kokkos execution space.
* `<start_time_ms>`: Epoch timestamp in milliseconds when the kernel started.
* `<end_time_ms>`: Epoch timestamp in milliseconds when the kernel ended.
* `<duration_ms>`: Duration of the kernel execution in milliseconds.

Example:

```
KERNEL_TIMING,For,"MyVectorAdd",1678886400100,1678886400150,50
KERNEL_TIMING,Reduce,"DotProduct",1678886400200,1678886400210,10
```

## Building and Usage

To use this profiler, you need to compile it as a Kokkos Tools library and then load it with your Kokkos application.

### Prerequisites

* **Kokkos**: Ensure you have Kokkos installed and configured.
* **Variorum**: The Variorum library must be installed on your system. This profiler links against `libvariorum`.
    * Set the `VARIORUM_ROOT` environment variable to the installation path of Variorum.
* **Jansson**: The Jansson C library for JSON parsing is required, as Variorum outputs data in JSON format.
* **C++20 Compiler**: The profiler uses `std::jthread` which requires C++20.
* **MPI Compiler**: The Makefile uses `mpicxx`, so an MPI-enabled C++ compiler is expected.

### Compilation

The provided `Makefile` simplifies the compilation process. Navigate to the directory containing the `Makefile` and run:

```bash
make
```

This will produce `power-profiler.so` in the same directory.

### Running Your Kokkos Application with the Profiler

To load the profiler with your Kokkos application, set the `KOKKOS_TOOLS_LIBS` environment variable to the path of the compiled shared library:

```bash
export KOKKOS_TOOLS_LIBS=/path/to/power-profiler.so
./your_kokkos_application
```

The power and kernel timing data will be printed to standard output upon application finalization. You can redirect this output to a file for later analysis:

```bash
export KOKKOS_TOOLS_LIBS=/path/to/power-profiler.so
./your_kokkos_application | tee profiling_output.txt
```