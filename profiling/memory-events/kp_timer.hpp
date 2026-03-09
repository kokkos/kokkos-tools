// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

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
