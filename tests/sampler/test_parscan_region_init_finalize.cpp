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

struct ParScanRegionFunctor {
  KOKKOS_FUNCTION void operator()(const int, long int&, bool) const {}
};

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_scan combined
 *       with regions. The print_ascii output is captured and validated.
 */

TEST(SimpleKernelTimerParScanRegion_DeathTest, parscan_region) {
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "KokkosP: Simple Kernel Timer Library Initialized");

  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "test region.*\\(Region\\)");

  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "named kernel scan.*\\(ParScan\\)");

  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "Total Calls to Kokkos Kernels:[ ]+5");

  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "Total Execution Time \\(incl. Kokkos \\+ "
      "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds");

  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N,
                                ParScanRegionFunctor{}, result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      "\\(ParScan\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+");
}
