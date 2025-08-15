# Energy Profiler Infrastructure
# This provides the foundational infrastructure for energy profiling tools:
# - Timer system for kernel and region tracking
# - Common utilities (filename generation, error handling)
# - Tool interface definitions
# - Basic kernel timer tool

option(ENABLE_ENERGY_PROFILER_VERBOSE "Enable verbose output for energy profiler tools" OFF)

add_subdirectory(unit-tests)
add_subdirectory(kokkos-tools)