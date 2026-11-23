# Energy Profiler Infrastructure with Variorum Provider
# Complete infrastructure for Variorum-based CPU/system energy profiling:
# - Timer system for kernel and region tracking
# - Common utilities (filename generation, error handling)
# - Tool interface definitions
# - Basic kernel timer tool
# - Daemon system for background monitoring tasks
# - Variorum provider for CPU/system power monitoring
# - Variorum-based profiling tool

option(KOKKOS_TOOLS_ENABLE_ENERGY_PROFILER_VERBOSE "Enable verbose output for energy profiler tools" OFF)

add_subdirectory(unit-tests)
add_subdirectory(kokkos-tools)