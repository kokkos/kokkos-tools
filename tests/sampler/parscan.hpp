// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#pragma once

struct Tester {
  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    //! Explicitly launch a kernel with a name, and run it 15 times with kernel
    //! logger. Use a periodic sampling with skip rate 5. This should print
    //! out 2 invocations, and there is a single matcher with a regular
    //! expression to check this.

    long int N = 1024;
    long int result;

    for (int iter = 0; iter < 15; iter++) {
      result = 0;
      Kokkos::parallel_scan("named kernel scan",
                            Kokkos::RangePolicy<execution_space>(space, 0, N),
                            *this, result);
    }
  }

  KOKKOS_FUNCTION void operator()(const int, long int&, bool) const {}
};
