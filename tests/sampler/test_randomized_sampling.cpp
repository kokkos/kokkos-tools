#include <iostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

struct Tester {
  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    //! Explicitly launch a kernel with a name, and run it 4 times with kernel
    //! logger. Use a periodic sampling with skip rate 2. This should print out
    //! 1 invocation In the test, we match the invocations output from the
    //! second invocation and make sure the first
    // and third are not printed out, and the second and fourth are sequenced
    // correctly in the print.

    for (int iter = 0; iter < 4; iter++) {
      Kokkos::parallel_for("named kernel",
                           Kokkos::RangePolicy<execution_space>(space, 0, 1),
                           *this);
    }

    KOKKOS_FUNCTION void operator()(const int) const {}
  };

  static const std::vector<std::string> matchers{
      "[0-9,a-z]+ KokkosP: sample 2 [0-9]+ KokkosP: sample 4 [0-9,a-z]+"};

  /**
   * @test This test checks that the tool effectively samples randomly.
   *

   */
  TEST(SamplerTest, randomized) {
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
    }  // end TEST
  }
