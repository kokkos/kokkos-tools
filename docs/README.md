# Kokkos Tools Documentation

This directory contains centralized documentation for Kokkos Tools and its connectors.

## Overview

Kokkos Tools provide a set of light-weight profiling and debugging utilities that interface with instrumentation hooks built directly into the Kokkos runtime. This documentation serves as a comprehensive guide for users and developers.

## Documentation Structure

- **[Connectors](connectors/)**: Documentation for third-party library (TPL) connectors

## Third-Party Library Connectors

Third-party library connectors (also known as advanced TPL connectors or aTPLs) are external profiling and analysis libraries maintained by third parties that provide their own connectors for the Kokkos ecosystem. These connectors allow Kokkos applications to leverage sophisticated profiling, tracing, and analysis capabilities from established HPC tools.

### Available TPL Connector Documentation

- [**ScoreP**](connectors/ScoreP.md) - Performance measurement infrastructure for parallel applications with profiling and tracing capabilities
- [**Timemory**](connectors/Timemory.md) - Modular performance analysis library with extensive component support
- [**Caliper**](connectors/Caliper.md) - Flexible and composable performance measurement framework from LLNL

### Contributing Documentation

If you maintain a third-party connector for Kokkos Tools, you can contribute documentation by:

1. Using the [documentation template](connectors/TEMPLATE.md) as a starting point
2. Creating a markdown file in the `docs/connectors/` directory
3. Following the structure of existing connector documentation
4. Opening a pull request to the Kokkos Tools repository

See the [ScoreP documentation](connectors/ScoreP.md) as a reference example.

## Additional Resources

- [Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki)
- [Main README](../README.md)
- [Building Kokkos Tools](../Build.md)
