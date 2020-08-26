#include <Kokkos_Core.hpp>

int main(){
  Kokkos::initialize();
  {
    Kokkos::View<float*, Kokkos::CudaSpace> dogs("puppies",100);
    Kokkos::View<float*, Kokkos::CudaSpace> dogs2("puppies",100);
    Kokkos::parallel_for("initialization",100,KOKKOS_LAMBDA(int i){
      dogs(i) = i;
      dogs2(i) = 2*i;
    });
    Kokkos::parallel_for("dogs",100,KOKKOS_LAMBDA(int i){
      dogs(100000+i) = i; //hopefully an access violation
    });
    dogs(0) = 1000; // definitely an access violation
  }
  Kokkos::finalize();
}
