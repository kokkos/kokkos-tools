# timemory Connector

[timemory](https://github.com/NERSC/timemory) is a __modular__ performance measurement and analysis library.

- Timing
    - wall-clock, cpu-clock, thread-cpu-clock, cpu-utilization, and more
- Memory
    - peak resident set size, page resident set size
- Resource Usage
    - page faults, bytes written, bytes read, context switches, etc.
- Hardware Counters
    - PAPI for CPUs
    - CUPTI for NVIDIA GPUs
- [Roofline Performance Model](https://docs.nersc.gov/programming/performance-debugging-tools/roofline/)
    - Requires PAPI for CPUs
    - Requires CUPTI for NVIDIA GPUs
- TAU instrumentation
- NVTX instrumentation
- LIKWID instrumentation
- VTune instrumentation
- Caliper instrumentation
- gperftools instrumentation

Kokkos support is built-in and more information can be found in the [documentation](https://timemory.readthedocs.io/en/develop/tools/kokkos-connector/README.html).
