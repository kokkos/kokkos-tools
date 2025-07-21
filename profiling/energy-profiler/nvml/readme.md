# NVML Power Profiler

A Kokkos profiling tool that uses NVML to collect power data from NVIDIA GPUs.

## Setup

Requires CUDA toolkit with NVML.
1. Install the CUDA toolkit that includes NVML.
2. Compile this module with the main CMake build.

## Output Files

- `hostname-pid-nvml-power.csv`: Power measurements
  - Format: `name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns`
- `hostname-pid-nvml-regions.csv`: Region timings
  - Format: `name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns`
- `hostname-pid-nvml-kernels.csv`: Kernel timings
  - Format: `name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns`

## Usage

```bash
export KOKKOS_PROFILE_LIBRARY=/path/to/kp_power_nvml.so
./your_kokkos_application
```

Interval of sampling power data can be controlled via the `KOKKOS_NVML_POWER_INTERVAL` environment variable, which specifies the interval in microseconds.