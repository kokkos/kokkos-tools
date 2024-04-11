#pragma once

struct Tester {
  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    //! Explicitly launch a kernel with a name, and run it 15 times with kernel
    //! logger. Use a periodic sampling with skip rate 5. This should print
    //! out 2 invocations, and there is a single matcher with a regular
    //! expression to check this.

    long int sum;
    for (int iter = 0; iter < 15; iter++) {
      sum = 0;
      Kokkos::parallel_reduce("named kernel reduce",
                              Kokkos::RangePolicy<execution_space>(space, 0, 1),
                              *this, sum);
    }
  }

  KOKKOS_FUNCTION void operator()(const int, long int&) const {}
};
