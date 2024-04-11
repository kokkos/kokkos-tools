#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

using ::testing::HasSubstr;
using ::testing::Not;

#include "parreduce.hpp"
#include "matchersSkip.hpp"

/*
 * @test This test checks that the sampling utility properly samples.
 *
 */

TEST(SamplerSkipTest, parreduce) {
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
    EXPECT_THAT(output.str(), HasSubstr(matcher));
  }

  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 1 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 2 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 3 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 4 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 5 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 7 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 8 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 9 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 10 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 11 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 13 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 14 calling")));
  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: sample 15 calling")));

  int occurrences            = 0;
  std::string::size_type pos = 0;
  std::string samplerTestOutput(output.str());
  std::string target("calling child-begin function");
  while ((pos = samplerTestOutput.find(target, pos)) != std::string::npos) {
    ++occurrences;
    pos += target.length();
  }

  EXPECT_EQ(occurrences, 2);

  EXPECT_THAT(output.str(), Not(HasSubstr("KokkosP: FATAL: No child library of "
                                          "sampler utility library to call")));

  EXPECT_THAT(output.str(),
              Not(HasSubstr("KokkosP: FATAL: Kokkos Tools Programming "
                            "Interface's tool-invoked Fence is NULL!")));
}
