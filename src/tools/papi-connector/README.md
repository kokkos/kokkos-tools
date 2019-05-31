### Build papi connector for kokkos

export PAPI_ROOT=<Path of PAPI>  
make clean  
make  

### Run kokkos application with PAPI recording enabled

export KOKKOS_PROFILE_LIBRARY=kp_papi_connector.so  

