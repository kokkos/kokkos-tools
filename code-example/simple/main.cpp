#include <Kokkos_Core.hpp>
#include <cmath>

using namespace Kokkos;

using exec_t = DefaultExecutionSpace;
using h_exec_t = DefaultHostExecutionSpace;

int main(int argc, char* argv[]) {
  initialize(argc, argv);
  {
     int N = 1000000;
     View<double*> a("A",N);
     View<double*, HostSpace> h_a = 
       create_mirror_view(a);


     Profiling::pushRegion("Setup");
     parallel_for("Init_A",RangePolicy<h_exec_t>(0,N),
       KOKKOS_LAMBDA(int i) { h_a(i) = i; });

     deep_copy(a,h_a);
     Profiling::popRegion();

     Profiling::pushRegion("Iterate");
     for(int r=0; r<10; r++) {
       View<double*> tmp("Tmp",N);
       parallel_scan("K_1",RangePolicy<exec_t>(0,N), 
         KOKKOS_LAMBDA(int i, double& lsum, bool f) {
         if(f) tmp(i) = lsum;
         lsum += a(i);
       });

       double sum;
       parallel_reduce("K_2",N, KOKKOS_LAMBDA(int i, double& lsum) {
         lsum += tmp(i);
       },sum);
       printf("%i %e\n",r,sum);
     }
     Profiling::popRegion();
  }
  Kokkos::finalize();
}
