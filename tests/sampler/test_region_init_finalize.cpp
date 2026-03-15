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

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for regions.
 *       The print_ascii output is captured and validated.
 */

TEST(SimpleKernelTimerRegion_DeathTest, region) {
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          ContainsRegex("test region.*\\(Region\\)"),
          ContainsRegex(
              "\\(Region\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          // Regions are not counted as kernel calls
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+0")));
}
