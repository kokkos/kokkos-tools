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

struct ParForRegionFunctor {
  KOKKOS_FUNCTION void operator()(const int) const {}
};

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_for combined
 *       with regions. The print_ascii output is captured and validated.
 */

TEST(SimpleKernelTimerParForRegion_DeathTest, parfor_region) {
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          Kokkos::parallel_for("named kernel",
                               Kokkos::RangePolicy<>(0, 1),
                               ParForRegionFunctor{});
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          // Validate both region and kernel types appear
          ContainsRegex("test region.*\\(Region\\)"),
          ContainsRegex("named kernel.*\\(ParFor\\)"),
          // Only parallel_for calls count as kernel calls, not regions
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          // Validate non-negative timing in kernel row
          ContainsRegex(
              "\\(ParFor\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}
