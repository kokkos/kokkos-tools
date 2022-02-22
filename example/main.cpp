#include <iostream>
#include <Kokkos_Core.hpp>
#include "kp_all.hpp"
#include "kernels.hpp"

#if USE_MPI
#include <mpi.h>
#endif

//-------------------------------------------------------------------------------------//

int main(int argc, char *argv[])
{
#if USE_MPI
  MPI_Init(&argc, &argv);
#endif

  const char *profiler_name = argc >= 2 ? argv[1] : "";
  const char *profiler_config = argc >= 3 ? argv[2] : "";

  auto eventSet = KokkosTools::get_event_set(profiler_name, profiler_config);

  // Note: callbacks must be set before Kokkos::initialize()
  Kokkos::Tools::Experimental::set_callbacks(eventSet);
  Kokkos::initialize(argc, argv);

  Kokkos::print_configuration(std::cout);

  std::cout << std::endl;
  int ret_code = run_calculation(100000);
  std::cout << std::endl;

  Kokkos::finalize();
#if USE_MPI
  MPI_Finalize();
#endif

  return ret_code;
}

//-------------------------------------------------------------------------------------//
