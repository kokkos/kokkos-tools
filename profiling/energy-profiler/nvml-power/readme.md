# NVML Power Profiler

A Kokkos profiling tool that uses NVML to collect power data from NVIDIA GPUs.
This variant uses `nvmlDeviceGetTotalEnergyConsumption(device, &energy)` to get energy data for kernels and regions.

## Setup

Requires CUDA toolkit with NVML.
1. Install the CUDA toolkit that includes NVML.
2. Compile this module with the main CMake build.

## Output Files

- `hostname-pid-nvml-power-kernels.csv`: Kernel power measurements
  - Format: `name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns,start_energy_mj,end_energy_mj,delta_energy_mj,average_power_w`
- `hostname-pid-nvml-power-regions.csv`: Region timings
  - Format: `name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns,start_energy_mj,end_energy_mj,delta_energy_mj,average_power_w`

## Usage

```bash
export KOKKOS_PROFILE_LIBRARY=/path/to/kp_power_nvml.so
./your_kokkos_application
```