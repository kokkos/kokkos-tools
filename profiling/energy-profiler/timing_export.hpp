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

#pragma once

#include <string>
#include <vector>
#include "timing_utils.hpp"
#include "power_sampler.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

void export_all_timings_csv(const std::vector<TimingInfo>& all_timings,
                            const std::string& filename);
void print_all_timings_summary(std::ostream& os,
                               std::vector<TimingInfo>::const_iterator begin,
                               std::vector<TimingInfo>::const_iterator end);

void export_power_data_csv(const std::vector<PowerSample>& samples,
                           const std::string& filename);
void print_power_summary(const std::vector<PowerSample>& samples,
                         const std::string& device_name = "N/A");

}  // namespace EnergyProfiler
}  // namespace KokkosTools
