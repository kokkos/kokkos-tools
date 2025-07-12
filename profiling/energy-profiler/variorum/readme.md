# Variorum Energy Profiler

A Kokkos profiling tool that uses Variorum to collect power measurements from supported hardware.

## Setup

1. Install Variorum library
2. Compile this module with Variorum using the main CMake build system.

## Configuration

Environment variables:
- `KOKKOS_TOOLS_POWER_MONITOR_INTERVAL`: Sampling interval in microseconds (default: 20000)
- `KOKKOS_TOOLS_POWER_OUTPUT_PATH`: Base path for output files (optional)

## Output Files

The profiler generates three CSV files:
- `hostname-pid-power.csv`: Raw power readings with absolute epoch timestamps
  - Format: `timestamp_epoch_ns,device_id,power_watts`
- `hostname-pid-regions.csv`: Timing for user-defined regions
  - Format: `name,type,start_timestamp_epoch_ns,end_timestamp_epoch_ns,duration_ns`
- `hostname-pid-kernels.csv`: Timing for Kokkos kernels
  - Format: `name,type,start_timestamp_epoch_ns,end_timestamp_epoch_ns,duration_ns,kernel_id`

Power readings are in watts and timestamps are in nanoseconds since the epoch.

## Usage

```bash
export KOKKOS_PROFILE_LIBRARY=/path/to/variorum_energy_profiler.so
./your_kokkos_application
```

> Note: You might need to set the `LD_LIBRARY_PATH` to include the Variorum library path if it's not in a standard location:
> ```bash
> export LD_LIBRARY_PATH=/path/to/variorum/lib:$LD_LIBRARY_PATH
> ```