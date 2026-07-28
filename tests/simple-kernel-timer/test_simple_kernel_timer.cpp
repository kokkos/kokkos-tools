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

struct ParReduceFunctor {
  KOKKOS_FUNCTION void operator()(const int, long int&) const {}
};

struct ParScanFunctor {
  KOKKOS_FUNCTION void operator()(const int, long int&, bool) const {}
};

/**
 * @brief Test fixture for simple kernel timer death tests.
 *
 * Death tests fork a child process via EXPECT_EXIT. The dup2 call that
 * redirects stdout to stderr must be performed inside the child process
 * (i.e., inside the EXPECT_EXIT block) because SetUp() runs in the parent
 * process and would not affect the child's file descriptors. Therefore,
 * dup2 remains inside each EXPECT_EXIT block.
 */
class SimpleKernelTimer_DeathTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // dup2(STDERR_FILENO, STDOUT_FILENO) cannot be moved here because
    // SetUp() runs in the parent process, while EXPECT_EXIT forks a child
    // process. The redirect must happen inside the EXPECT_EXIT block.
  }
};

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_for.
 *       The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parfor) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        for (int i = 0; i < 5; i++) {
          Kokkos::parallel_for("named kernel", Kokkos::RangePolicy<>(0, 1),
                               ParForFunctor{});
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          HasSubstr("Total Time, Call Count, Avg. Time per Call"),
          ContainsRegex("named kernel.*\\(ParFor\\)"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex(
              "\\(ParFor\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_reduce.
 *       The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parreduce) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int sum;
        for (int i = 0; i < 5; i++) {
          sum = 0;
          Kokkos::parallel_reduce("named kernel reduce",
                                  Kokkos::RangePolicy<>(0, 1),
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

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_scan.
 *       The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parscan) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N, ParScanFunctor{},
                                result);
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          HasSubstr("Total Time, Call Count, Avg. Time per Call"),
          ContainsRegex("named kernel scan.*\\(ParScan\\)"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex(
              "\\(ParScan\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for regions.
 *       The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, region) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
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

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_for combined
 *       with regions. The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parfor_region) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          Kokkos::parallel_for("named kernel", Kokkos::RangePolicy<>(0, 1),
                               ParForFunctor{});
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          ContainsRegex("test region.*\\(Region\\)"),
          ContainsRegex("named kernel.*\\(ParFor\\)"),
          // Only parallel_for calls count as kernel calls, not regions
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex(
              "\\(ParFor\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_reduce combined
 *       with regions. The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parreduce_region) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int sum;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          sum = 0;
          Kokkos::parallel_reduce("named kernel reduce",
                                  Kokkos::RangePolicy<>(0, 1),
                                  ParReduceFunctor{}, sum);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          ContainsRegex("test region.*\\(Region\\)"),
          ContainsRegex("named kernel reduce.*\\(ParRed\\)"),
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex(
              "\\(ParRed\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}

/**
 * @test This death test checks that kokkosp_init_library and
 *       kokkosp_finalize_library work correctly for parallel_scan combined
 *       with regions. The print_ascii output is captured and validated.
 */
TEST_F(SimpleKernelTimer_DeathTest, parscan_region) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_EXIT(
      {
        dup2(STDERR_FILENO, STDOUT_FILENO);

        Kokkos::initialize();

        long int N = 1024;
        long int result;
        for (int i = 0; i < 5; i++) {
          Kokkos::Profiling::pushRegion("test region");
          result = 0;
          Kokkos::parallel_scan("named kernel scan", N, ParScanFunctor{},
                                result);
          Kokkos::Profiling::popRegion();
        }

        Kokkos::finalize();
        exit(0);
      },
      ::testing::ExitedWithCode(0),
      ::testing::AllOf(
          HasSubstr("KokkosP: Simple Kernel Timer Library Initialized"),
          ContainsRegex("test region.*\\(Region\\)"),
          ContainsRegex("named kernel scan.*\\(ParScan\\)"),
          ContainsRegex("Total Calls to Kokkos Kernels:[ ]+5"),
          ContainsRegex("Total Execution Time \\(incl. Kokkos \\+ "
                        "non-Kokkos\\):[ ]+[0-9]+\\.[0-9]+ seconds"),
          ContainsRegex(
              "\\(ParScan\\)[ ]+[0-9]+\\.[0-9]+[ ]+5[ ]+[0-9]+\\.[0-9]+")));
}
