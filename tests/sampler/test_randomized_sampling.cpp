#include <iostream>
#include <sstream>
#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

struct Tester {
  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    //! Explicitly launch a kernel with a name, and run it 150 times with kernel
    //! logger. Use a periodic sampling with skip rate 101. This should print out
    //! 1 invocation In the test, we match the invocations output from the
    //! second invocation and make sure the first
    // and third are not printed out, and the second and fourth are sequenced
    // correctly in the print.

    for (int iter = 0; iter < 150; iter++) {
      Kokkos::parallel_for("named kernel",
                           Kokkos::RangePolicy<execution_space>(space, 0, 1),
                           *this);
    }

    KOKKOS_FUNCTION void operator()(const int) const {}
  };

  static const std::vector<std::string> matchers{
  "> (.*)\| KokkosP: sample 100 calling child-begin function...\nKokkosP: Sampler attempting to invoke tool-induced fence on device 0.\nKokkosP: Sampler sucessfully invoked tool-induced fence on device 0\n > (.*)\| KokkosP: sample 100 finished with child-begin function.\nKokkosP: sample 100 calling child-end function...\nKokkosP: Sampler attempting to invoke tool-induced fence on device 0.\nKokkosP: Sampler sucessfully invoked tool-induced fence on device 0\n > (.*)\|" 
};

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
