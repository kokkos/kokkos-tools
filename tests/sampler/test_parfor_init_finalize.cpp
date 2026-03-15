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

struct ParForFunctor {
  KOKKOS_FUNCTION void operator()(const int) const {}
};

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_for.
 *       The print_ascii output is captured and validated.
 */

TEST(SimpleKernelTimerParFor_DeathTest, parfor) {
  EXPECT_EXIT(
      {
        // Redirect stdout to stderr so the death test captures the output.
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        for (int i = 0; i < 5; i++) {
          Kokkos::parallel_for("named kernel",
                               Kokkos::RangePolicy<>(0, 1), ParForFunctor{});
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          // Validate init message
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          // Validate print_ascii header
          HasSubstr("Total Time, Call Count, Avg. Time per Call"),
          // Validate kernel name and type
          ContainsRegex("named kernel.*\\(ParFor\\)"),
          // Validate summary section with non-negative timing numbers
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          // Validate total kernel calls = 5
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          // Validate non-negative timing in kernel row
          ContainsRegex(
              "\\(ParFor\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}
