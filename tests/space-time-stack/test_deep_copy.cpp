#include <iostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

struct Tester {
  struct TagNamed {};
  struct TagUnnamed {};

  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    Kokkos::View<double*, execution_space> a("view_a", 10);
    Kokkos::View<double*, execution_space> b("view_b", 10);

    Kokkos::deep_copy(a, 1.5);
    Kokkos::deep_copy(space, b, 2.0);
    Kokkos::deep_copy(space, b, a);
  }
};

static const std::vector<std::string> matchers{
    // copy of scalar into view_a
    "[0-9.e]+ sec [0-9.]+% [0-9.]+% 0.0% ------ 1 \"view_a\"=\"Scalar\" "
    "\\([A-Z]+->[A-Z]+\\) \\[copy\\]",
    // copy of scalar into view_b, execution space overload which apparently
    // reports (none) for the source instead of Scalar
    "[0-9.e]+ sec [0-9.]+% [0-9.]+% 0.0% ------ 1 \"view_b\"=\".*\" "
    "\\([A-Z]+->[A-Z]+\\) \\[copy\\]",
    // copy of view_a into view_b
    "[0-9.e]+ sec [0-9.]+% [0-9.]+% 0.0% ------ 1 \"view_b\"=\"view_a\" "
    "\\([A-Z]+->[A-Z]+\\) \\[copy\\]",
};

/**
 * @test This test checks that the tool outputs deep_copy statistics.
 */
TEST(SpaceTimeStackTest, deep_copy) {
  //! Initialize @c Kokkos.
  Kokkos::initialize();

  //! Redirect output for later analysis.
  std::cout.flush();
  std::ostringstream output;
  std::streambuf* coutbuf = std::cout.rdbuf(output.rdbuf());

  //! Run tests. @todo Replace this with Google Test.
  Tester tester(Kokkos::DefaultExecutionSpace{});

  //! Finalize @c Kokkos.
  Kokkos::finalize();

  //! Restore output buffer.
  std::cout.flush();
  std::cout.rdbuf(coutbuf);
  std::cout << output.str() << std::endl;

  //! Analyze test output.
  for (const auto& matcher : matchers) {
    EXPECT_THAT(output.str(), ::testing::ContainsRegex(matcher));
  }
}
