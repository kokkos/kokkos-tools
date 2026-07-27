#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"
#include "Kokkos_Profiling_ScopedRegion.hpp"
#include "mpi.h"

struct Tester {
  explicit Tester() {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // The threshold for filtering out lines from the stack is 0.1% of the total
    // runtime. Do a 1s sleep on all ranks, then a 1.5 ms sleep on only rank 1.
    // If the threshold is applied based on the average time across ranks the
    // second region will be filtered out, but if it is based on the max time
    // then it will be included (which is the desired behavior so that we see
    // things which are slow only on a subset of ranks in cases like a coarse
    // solve in a multigrid method that only runs on a subset of the ranks).
    {
      Kokkos::Profiling::ScopedRegion all_ranks_region("all_ranks_region");
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    {
      Kokkos::Profiling::ScopedRegion rank_1_region("rank_1_region");
      if (rank == 1) {
        std::this_thread::sleep_for(std::chrono::microseconds(1500));
      }
    }
  }
};

static const std::vector<std::string> matchers{
    "[0-9.e]+ sec [0-9.]+% [0-9.]+% 0.[0-9]+% 100.0% 0.00e\\+00 1 "
    "all_ranks_region \\[region\\]",
    "[0-9.e]+ sec [0-9.]+% [0-9.]+% [1-9][0-9.]+% 100.0% 0.00e\\+00 1 "
    "rank_1_region \\[region\\]",
};

/**
 * @test This test checks that the tool outputs deep_copy statistics.
 */
TEST(SpaceTimeStackTest, threshold_with_imbalance) {
  MPI_Init(nullptr, nullptr);
  int comm_size;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  ASSERT_EQ(2, comm_size) << " test requires exactly 2 MPI ranks.";

  //! Initialize @c Kokkos.
  Kokkos::initialize();

  //! Redirect output for later analysis.
  std::cout.flush();
  std::ostringstream output;
  std::streambuf* coutbuf = std::cout.rdbuf(output.rdbuf());

  Tester tester;

  //! Finalize @c Kokkos.
  Kokkos::finalize();

  //! Restore output buffer.
  std::cout.flush();
  std::cout.rdbuf(coutbuf);
  std::cout << output.str() << std::endl;

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //! Analyze test output.
  if (rank == 0) {
    for (const auto& matcher : matchers) {
      EXPECT_THAT(output.str(), ::testing::ContainsRegex(matcher));
    }
  }
  MPI_Finalize();
}
