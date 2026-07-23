// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <string>
#include <iostream>
#include <regex>
#include <unistd.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

using ::testing::HasSubstr;
using ::testing::Not;

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
      // Validate init message
      "KokkosP: Simple Kernel Timer Library Initialized");

  EXPECT_EXIT(
      {
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
      // Validate print_ascii header
      "Total Time, Call Count, Avg. Time per Call");

  EXPECT_EXIT(
      {
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
      // Validate kernel name and type
      "named kernel.*\\(ParFor\\)");

  EXPECT_EXIT(
      {
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
      // Validate summary section with non-negative timing numbers (%f format)
      "Total Execution Time \\(incl. Kokkos \\+ "
      "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds");

  EXPECT_EXIT(
      {
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
      // Validate total kernel calls = 5
      "Total Calls to Kokkos Kernels:[ ]+5");

  EXPECT_EXIT(
      {
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
      // Validate non-negative timing in kernel row (format: type delimiter time
      // delimiter count ...)
      "\\(ParFor\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+");
}
