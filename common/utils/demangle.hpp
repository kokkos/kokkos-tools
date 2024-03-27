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

#ifndef KOKKOSTOOLS_COMMON_UTILS_DEMANGLE_HPP
#define KOKKOSTOOLS_COMMON_UTILS_DEMANGLE_HPP

#include <string>

#if defined(__GXX_ABI_VERSION)
#define HAVE_GCC_ABI_DEMANGLE
#endif

#if defined(HAVE_GCC_ABI_DEMANGLE)
#include <cxxabi.h>
#endif  // HAVE_GCC_ABI_DEMANGLE

namespace KokkosTools {

//! Demangle @p mangled_name.
inline std::string demangleName(const std::string_view mangled_name) {
#if defined(HAVE_GCC_ABI_DEMANGLE)
  int status = 0;

  char* demangled_name =
      abi::__cxa_demangle(mangled_name.data(), nullptr, nullptr, &status);

  if (demangled_name) {
    std::string ret(demangled_name);
    std::free(demangled_name);
    return ret;
  }
#endif
  return std::string(mangled_name);
}

/**
 * @brief Demangle @p mangled_name.
 *
 * This function supports @c Kokkos convention from
 * @c Kokkos::Impl::ParallelConstructName.
 *
 * For instance, a kernel launched with a tag would appear as
 * "<functor type>/<tag type>".
 */
inline std::string demangleNameKokkos(const std::string_view mangled_name) {
  if (size_t pos = mangled_name.find('/', 0);
      pos != std::string_view::npos && pos > 0) {
    /// An explicit copy of the first part of the string is needed, because
    /// @c abi::__cxa_demangle will parse the pointer until its NULL-terminated.
    return demangleName(std::string(mangled_name.substr(0, pos)))
        .append("/")
        .append(
            demangleName(mangled_name.substr(pos + 1, mangled_name.size())));
  } else {
    return demangleName(mangled_name);
  }
}

}  // namespace KokkosTools

#endif  // KOKKOSTOOLS_COMMON_UTILS_DEMANGLE_HPP
