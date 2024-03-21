#include <iostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

#include "kp_all.hpp"
#include "../../profiling/space-time-stack/kp_space_time_stack.hpp"

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

//! Test setup for the 'space-time-stack' tool.
class SpaceTimeStackTest : public ::testing::Test {
 public:
  //! At the beginning of the test suite, try to add the related callbacks.
  static void SetUpTestSuite() {
    Kokkos::Tools::Experimental::set_callbacks(
        KokkosTools::get_event_set("space-time-stack", nullptr));
    Kokkos::initialize();
  }

  static void TearDownTestSuite() { Kokkos::finalize(); }

  /**
   * At test setup, finalize first and then initialize to cleanse
   * @ref KokkosTools::SpaceTimeStack::State::global_state.
   */
  void SetUp() override {
    KokkosTools::SpaceTimeStack::State::finalize();
    KokkosTools::SpaceTimeStack::State::initialize();
  }

  void TearDown() override { KokkosTools::SpaceTimeStack::State::finalize(); }
};

/**
 * @test This test checks that the tool effectively uses
 *       the demangling helpers.
 */
TEST_F(SpaceTimeStackTest, demangling) {
  //! Redirect output for later analysis.
  std::cout.flush();
  std::ostringstream output;
  std::streambuf* coutbuf = std::cout.rdbuf(output.rdbuf());

  //! Run tests. @todo Replace this with Google Test.
  Tester tester(Kokkos::DefaultExecutionSpace{});

  /// Finalizing will call @ref KokkosTools::SpaceTimeStack::State::~State
  /// that outputs in @c std::cout.
  KokkosTools::SpaceTimeStack::State::finalize();

  //! Restore output buffer.
  std::cout.flush();
  std::cout.rdbuf(coutbuf);
  std::cout << output.str() << std::endl;

  //! Analyze test output.
  for (const auto& matcher : matchers) {
    EXPECT_THAT(output.str(), ::testing::ContainsRegex(matcher));
  }
}
