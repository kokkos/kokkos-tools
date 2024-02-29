#include <iostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

#define NUMITERS 1000
struct Tester {
  struct TagNamed {};
  struct TagUnnamed {};

  template <typename execution_space>
  explicit Tester(const execution_space& space) {
    //! Explicitly launch a kernel with a name and no tag, and run it NUM_ITERS times
    //! Use a random sampling of 1 percent and use a random seed of 2.
    //! With NUMITERS 1000, this should print out 10 invocations. 
     //! In the test, we match the invocations output from all three runs against each other, and ensure that each shows 10 invocations. 

    for (int trial = 0; trial < 3; trial++)
    {
     for (int iter = 0; iter < NUMITERS; iter++) {
     Kokkos::parallel_for("named kernel",
                         Kokkos::RangePolicy<execution_space>(space, 0, 1),
                         *this);
     }
   }
  
  KOKKOS_FUNCTION void operator()(const int) const {}

  template <typename TagType>
  KOKKOS_FUNCTION void operator()(const TagType, const int) const {}
};

static const std::vector<std::string> matchers{
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
  } // end TEST 

} 
