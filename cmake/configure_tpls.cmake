# Note: Caliper docs https://software.llnl.gov/Caliper/build.html
#       might miss some options, check the top CMakeLists.txt:
#       https://github.com/LLNL/Caliper/blob/master/CMakeLists.txt
macro(configure_caliper)
  set(CALIPER_OPTION_PREFIX ON)
  set(CALIPER_BUILD_DOCS OFF)    # Build documentation.
  set(CALIPER_BUILD_TESTING OFF) # Build unit tests.
  set(CALIPER_WITH_FORTRAN OFF)  # Build and install Fortran wrappers.
  set(CALIPER_WITH_KOKKOS ON)    # Enable Kokkos profiling support

  # Note: Let Caliper figure that out or it may fail on missing omp-tools.h
  # set(CALIPER_WITH_OMPT OFF) # Build with support for the OpenMP tools interface.

  set(CALIPER_WITH_SAMPLER ON)  # Enable time-based sampling on Linux.
  set(CALIPER_WITH_TOOLS ON)    # Build Caliper’s tools (i.e, cali-query and mpi-caliquery). Default: On.
  set(CALIPER_WITH_MPI ${KOKKOS_ENABLE_MPI}) # Build with MPI support.
  set(CALIPER_WITH_ROCM ${Kokkos_ENABLE_HIP}) # Enable AMD ROCtracer/RocTX support
  set(CALIPER_WITH_NVTX ${Kokkos_ENABLE_CUDA}) # Build adapters to forward Caliper annotations to NVidia’s nvtx annotation API. Set CUDA_TOOLKIT_ROOT_DIR to the CUDA installation.
  set(CALIPER_WITH_CUPTI ${Kokkos_ENABLE_CUDA}) # Enable support for CUDA performance analysis
                                        # (wrapping of driver/runtime API calls and CUDA activity tracing).
  # Enable PAPI hardware counter service (requires papi)
  if(KokkosTools_ENABLE_PAPI)
    set(CALIPER_WITH_PAPI ON)
    set(PAPI_PREFIX ${PAPI_ROOT})
  else()
    set(CALIPER_WITH_PAPI OFF)
  endif()
  # Build adapters to forward Caliper annotations to Intel’s VTune annotation API.
  if(VTune_ROOT)
    set(CALIPER_WITH_VTUNE ON)
    set(ITT_PREFIX ${VTune_ROOT}/sdk)
    # TODO: Caliper doesn't seek ITT in ${VTune_ROOT}/sdk/lib64 or ${VTune_ROOT}/sdk/lib32 (only in .../lib)
  else()
    set(CALIPER_WITH_VTUNE OFF)
  endif()
endmacro()

# Note: Apex docs http://uo-oaciss.github.io/apex/install/#standalone_installation
macro(configure_apex)
  #set(BUILD_STATIC_EXECUTABLES OFF)
  set(APEX_WITH_PAPI ${KokkosTools_ENABLE_PAPI})
  set(APEX_WITH_CUDA ${Kokkos_ENABLE_CUDA})
  set(APEX_WITH_BFD ON)
  #set(BFD_ROOT ...)

  set(APEX_WITH_OMPT OFF) # TODO: Apex fails on missing libomp.so

  # TODO: Apex tries to fetch TPLs and install them in /usr/lib
  #       (normally failing) unless we disable them...
  set(APEX_WITH_ACTIVEHARMONY OFF)
  set(APEX_WITH_OTF2 OFF)
  set(APEX_WITH_MPI OFF)
endmacro()
