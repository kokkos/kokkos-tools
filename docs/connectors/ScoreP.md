# ScoreP Connector

## Overview

[ScoreP](https://www.score-p.org/) (Scalable Performance Measurement Infrastructure for Parallel Codes) is a highly scalable and easy-to-use performance measurement infrastructure for parallel applications. It provides comprehensive profiling and tracing capabilities that are essential for HPC performance analysis.

The ScoreP Kokkos Tools connector is a third-party library (TPL) connector that brings ScoreP's capabilities to Kokkos applications. The connector invokes ScoreP's measurement functions through callbacks corresponding to Kokkos library functions, such as `Kokkos::parallel_for()`, `Kokkos::parallel_reduce()`, and `Kokkos::parallel_scan()`.

## Key Features

- **Profiling**: Collect performance metrics for Kokkos kernels and regions
- **Tracing**: Generate detailed execution traces for visualization and analysis
- **Scalability**: Designed for large-scale parallel applications
- **Integration**: Seamless integration with Kokkos profiling hooks

## Use Cases

ScoreP is particularly valuable for:
- Understanding performance bottlenecks in Kokkos applications
- Analyzing parallel execution patterns
- Tracing communication and synchronization in distributed applications
- Performance tuning of HPC applications

Note: While ScoreP provides advanced tracing capabilities from a third-party library, it focuses on profiling and tracing rather than intelligent analysis or automatic tuning (unlike tools such as APEX).

## Installation and Setup

### Prerequisites

- A recent version of ScoreP (installation instructions available at [score-p.org](https://www.score-p.org/))
- Kokkos configured with profiling support (`Kokkos_ENABLE_LIBDL=ON`)

### Building the ScoreP Connector

The ScoreP connector can be obtained from the ScoreP project's GitHub repository. Detailed build and installation instructions are available in the connector's repository.

For information on building and using the ScoreP Kokkos Tools connector, please refer to:
- [ScoreP official documentation](https://perftools.pages.jsc.fz-juelich.de/cicd/scorep/tags/latest/html/measurement.html)
- The ScoreP connector repository (maintained externally by the ScoreP development team)

### Using the Connector

Once built, the ScoreP connector can be used like other Kokkos Tools connectors:

1. Build your Kokkos application as usual
2. Set the `KOKKOS_TOOLS_LIBS` environment variable to point to the ScoreP connector library
3. Configure ScoreP measurement settings through ScoreP-specific environment variables
4. Run your application

```bash
export KOKKOS_TOOLS_LIBS=/path/to/scorep/kokkos/connector.so
# Set ScoreP configuration variables as needed
./your_kokkos_application
```

## Resources and Documentation

### Official ScoreP Resources

- **Website**: [score-p.org](https://www.score-p.org/)
- **Documentation**: [ScoreP User Manual](https://perftools.pages.jsc.fz-juelich.de/cicd/scorep/tags/latest/html/measurement.html)
- **Tutorials**: ScoreP provides comprehensive tutorials for profiling and tracing parallel applications

### Kokkos-Specific Resources

- **Tutorial**: [Tools Tutorial from OLCF](https://www.olcf.ornl.gov/wp-content/uploads/2021/06/ToolsTutorialOLCF.pptx.pdf) - Includes information on using ScoreP with Kokkos applications
- **Wiki Page**: [ScoreP on Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki/ScoreP)

## Integration Plans

Future integration of the ScoreP Kokkos Tools connector into this repository may include:
- Git submodule integration for easier access
- Support in the Kokkos Tools Spack package
- Additional build system integration

## Support and Contributing

The ScoreP Kokkos Tools connector is maintained by the ScoreP development team. For issues or questions specific to the connector:
- Refer to the ScoreP project documentation and support channels
- For general Kokkos Tools questions, see the main [Kokkos Tools README](../../README.md)

## See Also

- [Timemory Connector](Timemory.md) - Another advanced TPL connector with extensive capabilities
- [Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki)
- [Main Kokkos Tools Documentation](../README.md)
