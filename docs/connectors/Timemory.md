# Timemory Connector

## Overview

[Timemory](https://github.com/NERSC/timemory) is a modular performance measurement and analysis library designed for flexible and highly-efficient performance analysis. It provides a template-based API that allows users to compose custom analysis tools from a rich set of components.

The Timemory connector for Kokkos Tools provides deep integration with Kokkos applications, offering comprehensive performance analysis capabilities through its modular component system.

## Key Features

Timemory provides a wide range of performance analysis capabilities:

### Timing
- Wall-clock, CPU-clock, thread-CPU-clock
- CPU utilization tracking
- And more timing components

### Memory Analysis
- Peak resident set size (RSS)
- Page resident set size
- Virtual memory tracking

### Resource Usage
- Page faults
- Bytes written and read
- Context switches
- Other system resource metrics

### Hardware Counters
- PAPI support for CPU hardware counters
- CUPTI support for NVIDIA GPU counters

### Roofline Performance Model
- CPU Roofline analysis (requires PAPI)
- GPU Roofline analysis (requires CUPTI)
- Empirical performance modeling

### Third-Party Tool Integration
- TAU instrumentation
- NVTX instrumentation
- LIKWID instrumentation
- VTune instrumentation
- Caliper instrumentation
- gperftools instrumentation

## Installation

### Installing Timemory

Timemory uses a standard CMake build system:

```bash
git clone https://github.com/NERSC/timemory.git timemory
mkdir build-timemory
cd build-timemory
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_STATIC_LIBS=OFF ../timemory
make -j8
make install -j8
```

**Recommended Options:**
- Toggle statistics settings as desired
- Build the Python interface for plotting Kokkos output: `-DTIMEMORY_BUILD_PYTHON=ON`
- Use `TIMEMORY_REQUIRE_PACKAGES=ON` to ensure all desired tools are included

### Building the Connector

The Timemory connector is built using CMake:

```bash
# Set CMAKE_PREFIX_PATH to include timemory installation
export CMAKE_PREFIX_PATH=/path/to/timemory/install:$CMAKE_PREFIX_PATH

# Configure and build
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
make
make install
```

Various `USE_<PACKAGE>` options can be configured to activate external libraries for hardware counters and other features.

## Usage

### Basic Usage

The default `kp_timemory.so` connector library uses the `KOKKOS_TIMEMORY_COMPONENTS` environment variable to specify measurement components:

```bash
export KOKKOS_TOOLS_LIBS=/path/to/kp_timemory.so
export KOKKOS_TIMEMORY_COMPONENTS="wall_clock,cpu_clock,peak_rss"
./your_application
```

### Pre-configured Connector Libraries

Building with `BUILD_CONFIG=ON` generates several pre-configured connector libraries:
- `kp_timemory_timers.so` - Wall-clock, CPU-clock, and CPU utilization
- `kp_timemory_memory.so` - Peak RSS, page RSS, and virtual memory
- And other specialized configurations

### Roofline Analysis

For roofline capabilities:

1. Set up the environment:
```bash
export KOKKOS_ROOFLINE=ON
export TIMEMORY_OUTPUT_PATH=<OUTPUT-DIR>
```

2. Run twice with different modes:
```bash
# First run: operational intensity
TIMEMORY_ROOFLINE_MODE=op ./your_application

# Second run: arithmetic intensity  
TIMEMORY_ROOFLINE_MODE=ai ./your_application
```

3. Generate roofline plot:
```bash
timemory-roofline -t gpu_roofline \
  -op timemory-output/gpu_roofline_counters.json \
  -ai timemory-output/gpu_roofline_activity.json
```

## Output

Output is located in `timemory-output/<DATE-TIME>` unless `KOKKOS_ROOFLINE` is set. The `<DATE-TIME>` format can be customized via `TIMEMORY_TIME_FORMAT` (default: `"%F_%I.%M_%p"`).

Output formats include:
- Console output (printed at end of application)
- Text files
- JSON files
- Plots (if Python interface is available and enabled)

## Custom Components

Timemory's modular design allows users to create custom analysis components. A minimal component requires:

```cpp
namespace tim {
namespace component {
struct trip_count : public base<trip_count, int64_t> {
    using value_type = int64_t;
    using this_type  = trip_count;
    using base_type  = base<this_type, value_type>;

    static std::string label() { return "trip_count"; }
    static std::string description() { return "Number of invocations"; }
    static value_type  record() { return 1; }

    value_type get() const { return accum; }
    value_type get_display() const { return get(); }

    void start() { value = record(); }
    void stop() { accum += value; }
};
}  // namespace component
}  // namespace tim
```

Components can be queried using the `timemory-avail` command-line tool.

## Documentation

Kokkos support is built-in to Timemory. For comprehensive documentation, see:
- [Timemory Kokkos Connector Documentation](https://timemory.readthedocs.io/en/develop/tools/kokkos-connector/README.html)
- [Timemory GitHub Repository](https://github.com/NERSC/timemory)
- [Timemory Documentation](https://timemory.readthedocs.io/)

## Local Documentation

For quick reference, see the connector-specific README:
- [Timemory Connector README](../../profiling/timemory-connector/README.md)

## See Also

- [ScoreP Connector](ScoreP.md) - Performance measurement infrastructure with profiling and tracing
- [Kokkos Tools Wiki - Timemory](https://github.com/kokkos/kokkos-tools/wiki/Timemory)
- [Main Kokkos Tools Documentation](../README.md)
