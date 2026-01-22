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

#include <cstddef>

namespace KokkosTools {
namespace EnergyProfiler {

// Sampling interval in milliseconds
constexpr int SAMPLING_INTERVAL_MS = 20;

// Buffer size for hostname
const size_t HOSTNAME_BUFFER_SIZE = 256;

// Table formatting constants for timing export
const int COLUMN_WIDTH_CATEGORY = 10;
const int COLUMN_WIDTH_NAME     = 32;
const int COLUMN_WIDTH_TYPE     = 14;
const int COLUMN_WIDTH_TIME     = 17;
const int COLUMN_WIDTH_DURATION = 13;

}  // namespace EnergyProfiler
}  // namespace KokkosTools
