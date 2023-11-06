#ifndef KOKKOSTOOLS_COMMON_FRAMETYPE_HPP
#define KOKKOSTOOLS_COMMON_FRAMETYPE_HPP

#include <ostream>
#include <string>

namespace KokkosTools {

enum FrameType {
    PARALLEL_FOR    = 0,
    PARALLEL_REDUCE = 1,
    PARALLEL_SCAN   = 2,
    REGION          = 3,
    COPY            = 4
};

//! A @ref FrameType::REGION is not a kernel.
inline constexpr bool is_a_kernel(const FrameType type) {
  switch (type) {
    case PARALLEL_FOR   : return true;
    case PARALLEL_REDUCE: return true;
    case PARALLEL_SCAN  : return true;
    case REGION         : return false;
    case COPY           : return true;
    default: throw type;
  }
}

inline std::string to_string(const FrameType t) {
  switch (t) {
    case PARALLEL_FOR   : return "PARALLEL_FOR";
    case PARALLEL_REDUCE: return "PARALLEL_REDUCE";
    case PARALLEL_SCAN  : return "PARALLEL_SCAN";
    case REGION         : return "REGION";
    case COPY           : return "COPY";
    default: throw t;
  }
}

inline std::ostream &operator<<(std::ostream &os, const FrameType kind) {
  switch (kind) {
    case PARALLEL_FOR   : os << "[for]"; break;
    case PARALLEL_REDUCE: os << "[reduce]"; break;
    case PARALLEL_SCAN  : os << "[scan]"; break;
    case REGION         : os << "[region]"; break;
    case COPY           : os << "[copy]"; break;
  };
  return os;
}

} // namespace KokkosTools

#endif // KOKKOSTOOLS_COMMON_FRAMETYPE_HPP
