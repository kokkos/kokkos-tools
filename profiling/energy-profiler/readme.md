# Energy Profiling Tools for Kokkos

Collection of tools for energy profiling in Kokkos applications.

## Available Tools

- **Variorum**: Multi-vendor support (NVIDIA, AMD, Intel)
- **NVML**: NVIDIA GPU specific

Each tool directory contains:
- Source code
- Build/usage instructions
- Documentation on output format

## Daemon Mechanism

A "daemon" mechanism is used to collect power data during Kokkos application execution. This allows for continuous power monitoring with minimal overhead or more generally for data sampling at a specified interval.