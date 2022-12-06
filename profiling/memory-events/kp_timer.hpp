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

#ifndef KOKKOS_TIMER_HPP
#define KOKKOS_TIMER_HPP

#include <chrono>

namespace Kokkos {

/** \brief  Time since construction */

class Timer {
 private:
  std::chrono::high_resolution_clock::time_point m_old;
  Timer(const Timer&);
  Timer& operator=(const Timer&);

 public:
  void reset() { m_old = std::chrono::high_resolution_clock::now(); }

  Timer() { reset(); }

  double seconds() const {
    std::chrono::high_resolution_clock::time_point m_new =
        std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::duration<double>>(m_new -
                                                                     m_old)
        .count();
  }
};

}  // namespace Kokkos

#endif /* #ifndef KOKKOS_TIMER_HPP */
