# Alter some Caliper defaults, based on Kokkos and Tools settings
# see https://software.llnl.gov/Caliper/build.html
macro(configure_caliper)
  set_cache(CALIPER_OPTION_PREFIX ON)
  set_cache(CALIPER_WITH_KOKKOS ON)
  if(USE_MPI)
    set_cache(CALIPER_WITH_MPI ON)
  endif()
  if(KokkosTools_HAS_PAPI)
    set(PAPI_PREFIX ${PAPI_ROOT})
    set_cache(CALIPER_WITH_PAPI ON)
  endif()
  if(KOKKOSTOOLS_HAS_VARIORUM)
    set_cache(CALIPER_WITH_VARIORUM ON)
    set(VARIORUM_PREFIX ${Variorum_ROOT})
  endif()
  if(KOKKOSTOOLS_HAS_VTUNE)
    set_cache(CALIPER_WITH_VTUNE ON)
    set(ITT_PREFIX ${VTune_ROOT})
  endif()
  if(Kokkos_FOUND)
    if(Kokkos_ENABLE_CUDA)
      # TODO: check if this works...
      set_cache(CALIPER_WITH_NVTX ON)
      set_cache(CALIPER_WITH_CUPTI ON)
    endif()
    if(Kokkos_ENABLE_HIP)
      # TODO: check if this works...
      set_cache(CALIPER_WITH_ROCTX ON)
      set_cache(CALIPER_WITH_ROCTRACER ON)
    endif()
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
