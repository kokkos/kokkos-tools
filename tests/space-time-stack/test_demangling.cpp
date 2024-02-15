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
    //! Explicitly launch a kernel with a name and no tag.
    Kokkos::parallel_for("named kernel",
                         Kokkos::RangePolicy<execution_space>(space, 0, 1),
                         *this);

    //! Explicitly launch a kernel with a name and a tag.
    Kokkos::parallel_for(
        "named kernel with tag",
        Kokkos::RangePolicy<execution_space, TagNamed>(space, 0, 1), *this);

    //! Explicitly launch a kernel with no name and no tag.
    Kokkos::parallel_for(Kokkos::RangePolicy<execution_space>(space, 0, 1),
                         *this);

    //! Explicitly launch a kernel with no name and a tag.
    Kokkos::parallel_for(
        Kokkos::RangePolicy<execution_space, TagUnnamed>(space, 0, 1), *this);
  }

  KOKKOS_FUNCTION void operator()(const int) const {}

  template <typename TagType>
  KOKKOS_FUNCTION void operator()(const TagType, const int) const {}
};

static const std::vector<std::string> matchers{
    /// A kernel with a given name appears with the given name, no matter
    /// if a tag was given.
    "[0-9.e]+ sec [0-9.]+% 100.0% 0.0% ------ 1 named kernel \\[for\\]",
    "[0-9.e]+ sec [0-9.]+% 100.0% 0.0% ------ 1 named kernel with tag "
    "\\[for\\]",
    //! A kernel with no name and no tag appears with a demangled name.
    "[0-9.e]+ sec [0-9.]+% 100.0% 0.0% ------ 1 Tester \\[for\\]\n",
    //! A kernel with no name and a tag appears with a demangled name.
    "[0-9.e]+ sec [0-9.]+% 100.0% 0.0% ------ 1 Tester/Tester::TagUnnamed "
    "\\[for\\]"};

/**
 * @test This test checks that the tool effectively uses
 *       the demangling helpers.
 */
TEST(SpaceTimeStackTest, demangling) {
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
