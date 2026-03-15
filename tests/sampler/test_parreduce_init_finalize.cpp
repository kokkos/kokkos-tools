// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <string>
#include <iostream>
#include <regex>
#include <unistd.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

using ::testing::ContainsRegex;
using ::testing::HasSubstr;

struct ParReduceFunctor {
  KOKKOS_FUNCTION void operator()(const int, long int&) const {}
};

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_reduce.
 *       The print_ascii output is captured and validated.
 */

TEST(SimpleKernelTimerParReduce_DeathTest, parreduce) {
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int sum;
        for (int i = 0; i < 5; i++) {
          sum = 0;
          Kokkos::parallel_reduce(
              "named kernel reduce", Kokkos::RangePolicy<>(0, 1),
              ParReduceFunctor{}, sum);
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          HasSubstr("Total Time, Call Count, Avg. Time per Call"),
          ContainsRegex("named kernel reduce.*\\(ParRed\\)"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex(
              "\\(ParRed\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}
