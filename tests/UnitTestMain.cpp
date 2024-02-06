#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

//! Main entry point for tests.
int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  auto success = RUN_ALL_TESTS();

  return success;
}
