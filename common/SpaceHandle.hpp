#ifndef KOKKOSTOOLS_COMMON_SPACEHANDLE_HPP
#define KOKKOSTOOLS_COMMON_SPACEHANDLE_HPP

#include "impl/Kokkos_Profiling_C_Interface.h"

#include <ostream>
#include <string>

namespace KokkosTools
{
//! A @c Kokkos space type has a unique name.
using SpaceHandle = Kokkos_Profiling_SpaceHandle;

//! Supported @c Kokkos spaces.
enum Space {
    HOST = 0,
    CUDA = 1,
    HIP  = 2,
    SYCL = 3,
    OMPT = 4
};

//! Number of supported spaces (size of @ref Space).
constexpr size_t NSPACES = 5;

//! Get @ref Space from @ref SpaceHandle.
Space get_space(SpaceHandle const& handle)
{
    switch(handle.name[0])
    {
        // Only 'CUDA' space starts with 'C'.
        case 'C':
            return Space::CUDA;
        // Only 'SYCL' space starts with 'S'.
        case 'S':
            return Space::SYCL;
        // Only 'OpenMPTarget' starts with 'O'.
        case 'O':
           return Space::OMPT;
        // Otherwise, it's either 'HIP' or 'HOST'.
        case 'H':
            if(handle.name[1] == 'I') return Space::HIP;
            else                      return Space::HOST;
        default:
            std::abort();
    }
}

//! Get the name of a @ref Space.
const char* get_space_name(const Space space) {
  switch (space) {
    case Space::HOST: return "HOST";
    case Space::CUDA: return "CUDA";
    case Space::SYCL: return "SYCL";
    case Space::OMPT: return "OpenMPTarget";
    case Space::HIP : return "HIP";
  }
  std::abort();
}

std::ostream& operator<<(std::ostream& out, const Space space) {
    return out << get_space_name(space);
}

} // namespace KokkosTools

#endif // KOKKOSTOOLS_COMMON_SPACEHANDLE_HPP
