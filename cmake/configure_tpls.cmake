# Note: Caliper docs https://software.llnl.gov/Caliper/build.html
#       might miss some options, check the top CMakeLists.txt:
#       https://github.com/LLNL/Caliper/blob/master/CMakeLists.txt
macro(configure_caliper)
  set(CALIPER_OPTION_PREFIX ON)
  set(CALIPER_BUILD_DOCS OFF)    # Build documentation.
  set(CALIPER_BUILD_TESTING OFF) # Build unit tests.
  set(CALIPER_WITH_FORTRAN OFF)  # Build and install Fortran wrappers.

  # Note: Let Caliper figure that out or it may fail on missing omp-tools.h
  # set(CALIPER_WITH_OMPT OFF) # Build with support for the OpenMP tools interface.

  if(NOT WIN32)
    set(CALIPER_WITH_SAMPLER ON)  # Enable time-based sampling on Linux.
  endif()
  set(CALIPER_WITH_TOOLS ON)    # Build Caliper’s tools (i.e, cali-query and mpi-caliquery). Default: On.
  set(CALIPER_WITH_MPI ${KokkosTools_ENABLE_MPI}) # Build with MPI support.
  if(Kokkos_FOUND)
    set(CALIPER_WITH_KOKKOS ON)                   # Enable Kokkos profiling support
    set(CALIPER_WITH_ROCM ${Kokkos_ENABLE_HIP})   # Enable AMD ROCtracer/RocTX support
    set(CALIPER_WITH_NVTX ${Kokkos_ENABLE_CUDA})  # Build adapters to forward Caliper annotations to NVidia’s
                                                  # nvtx annotation API.
    set(CALIPER_WITH_CUPTI ${Kokkos_ENABLE_CUDA}) # Enable support for CUDA performance analysis (wrapping of
                                                  # driver/runtime API calls and CUDA activity tracing).
  else()
    set(CALIPER_WITH_KOKKOS OFF)
    # TODO: detect CUDA / HIP ?
  endif()

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
    set(ITT_PREFIX ${VTune_ROOT})
  else()
    set(CALIPER_WITH_VTUNE OFF)
  endif()
endmacro()

# Alter some Apex defaults, based on Kokkos and Tools settings
# See http://uo-oaciss.github.io/apex/install/#standalone_installation
macro(configure_apex)
  if(BUILD_SHARED_LIBS)
    set_cache(BUILD_STATIC_EXECUTABLES OFF)
  else()
    set_cache(BUILD_STATIC_EXECUTABLES ON)
  endif()
  set_cache(APEX_WITH_PAPI ${KokkosTools_ENABLE_PAPI})
  set_cache(APEX_WITH_MPI ${KokkosTools_ENABLE_MPI})

  ## TODO: Build Binutils if not installed (detect?) and the compiler is NOT gcc/clang/icc (check CMake vars)
  # set(BFD_ROOT /path/to/binutils)
  # option(APEX_BUILD_BFD "Build Binutils library if not found" ON)

  ## TODO: Build OMPT if compilers >= [gcc/clang/icc] and we're NOT offloading to GPU
  ## Note: OMPT should work nice with Intel compiler
  # option(APEX_BUILD_OMPT "Build OpenMP runtime with OMPT if support not found" ON)

  if(Kokkos_ENABLE_CUDA)
    option(APEX_WITH_CUDA "Enable CUDA (CUPTI) support" ON)
    # TODO: check if we need to set CUPTI_ROOT and/or NVML_ROOT here
  endif()

  if(Kokkos_ENABLE_HIP)
    option(APEX_WITH_HIP "Enable HIP (ROCTRACER) support" ON)
    ## TODO: check/set paths (we can skip roctracer, rocprofiler, rocm_smi if they're located in ${ROCM_PATH})
    # set(ROCM_ROOT ${ROCM_PATH})
    # set(ROCTX_ROOT ${ROCM_PATH}/roctracer)
    # set(ROCTRACER_ROOT ${ROCM_PATH}/roctracer)
    # set(RSMI_ROOT ${ROCM_PATH}/rocm_smi)
  endif()
endmacro()
