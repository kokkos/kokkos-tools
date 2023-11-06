#ifndef KOKKOSTOOLS_COMMON_UTILS_TIME_HPP
#define KOKKOSTOOLS_COMMON_UTILS_TIME_HPP

#include <chrono>
#include <sys/time.h>

namespace KokkosTools
{
struct Now {
    using impl_t = std::chrono::time_point<std::chrono::high_resolution_clock>;
    impl_t impl;
};

inline Now now() {
    Now t;
    t.impl = std::chrono::high_resolution_clock::now();
    return t;
}

inline uint64_t operator-(Now b, Now a) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b.impl - a.impl)
        .count();
}

//! @todo Use @c chrono instead.
inline double seconds() {
    struct timeval now;
    gettimeofday(&now, NULL);

    return (double)(now.tv_sec + (now.tv_usec * 1.0e-6));
}

} // namespace KokkosTools

#endif // KOKKOSTOOLS_COMMON_UTILS_TIME_HPP