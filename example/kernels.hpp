#pragma once
//-------------------------------------------------------------------------------------//

// Sample computation: S(N) = 1 + 2 + 3 + ... + N
// Tests: regions, allocation, parallel for and reduction
template <typename data_type>
int run_calculation(const data_type SIZE)
{
  Kokkos::Profiling::pushRegion("Computation");

  Kokkos::View<data_type*> data(Kokkos::ViewAllocateWithoutInitializing("data"), SIZE);
  Kokkos::parallel_for("initialize()", SIZE, KOKKOS_LAMBDA(data_type i) {
    data(i) = i;
  });
  Kokkos::fence();

  data_type sum = 0;
  Kokkos::parallel_reduce("accumulate()", SIZE, KOKKOS_LAMBDA(data_type i, data_type &lsum) {
    lsum += 1 + data(i);
  }, sum);
  Kokkos::fence();

  Kokkos::Profiling::popRegion();

  // check results
  const data_type check = (SIZE + 1) * SIZE / 2;
  if (sum != check) {
    std::cout << "BAD result, got S(" << SIZE << ") = " << sum << " (expected " << check << ")" << std::endl;
    return 1;
  }
  std::cout << "Result OK: S(" << SIZE << ") = " << sum << std::endl;
  return 0;
}

//-------------------------------------------------------------------------------------//
