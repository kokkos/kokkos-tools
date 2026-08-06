//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#include <iostream>
#include <deque>
#include <chrono>
#include <thread>
#include "../common/timer_system.hpp"

using namespace KokkosTools::EnergyProfiler;

int main() {
  std::cout << "Testing CSV export functions..." << std::endl;

  KernelTimerTool timer;

  // Simulate some kernel operations
  timer.start_region("test_kernel_1", RegionType::ParallelFor, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  timer.end_region();

  timer.start_region("test_kernel_2", RegionType::ParallelReduce, 2);
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  timer.end_region();

  timer.start_region("test_region", RegionType::UserRegion, 3);
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  timer.end_region();

  timer.start_region("test_deepcopy", RegionType::DeepCopy, 4);
  std::this_thread::sleep_for(std::chrono::milliseconds(8));
  timer.end_region();

  // Test the CSV export functions
  const auto& kernels    = timer.get_kernel_timings();
  const auto& regions    = timer.get_region_timings();
  const auto& deepcopies = timer.get_deep_copy_timings();

  std::cout << "Found " << kernels.size() << " kernels" << std::endl;
  std::cout << "Found " << regions.size() << " regions" << std::endl;
  std::cout << "Found " << deepcopies.size() << " deep copies" << std::endl;

  // Test export functions
  KokkosTools::EnergyProfiler::export_timings_csv(
      kernels, "test_kernels.csv",
      KokkosTools::EnergyProfiler::DataCategory::Kernels);
  KokkosTools::EnergyProfiler::export_timings_csv(
      regions, "test_regions.csv",
      KokkosTools::EnergyProfiler::DataCategory::Regions);
  KokkosTools::EnergyProfiler::export_timings_csv(
      deepcopies, "test_deepcopies.csv",
      KokkosTools::EnergyProfiler::DataCategory::DeepCopies);

  // Test print functions
  KokkosTools::EnergyProfiler::print_timings_summary(
      kernels, KokkosTools::EnergyProfiler::DataCategory::Kernels);
  KokkosTools::EnergyProfiler::print_timings_summary(
      regions, KokkosTools::EnergyProfiler::DataCategory::Regions);
  KokkosTools::EnergyProfiler::print_timings_summary(
      deepcopies, KokkosTools::EnergyProfiler::DataCategory::DeepCopies);

  std::cout << "CSV export test completed successfully!" << std::endl;

  return 0;
}
