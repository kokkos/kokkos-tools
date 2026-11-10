#ifndef _H_KOKKOS_LDMS_CONNECTOR_TIMER
#define _H_KOKKOS_LDMS_CONNECTOR_TIMER

#include <sys/time.h>

double seconds() {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  const double d_secs  = static_cast<double>(now.tv_sec);
  const double d_nsecs = static_cast<double>(now.tv_nsec) * 1.0e-9;

  return (d_secs + d_nsecs);
}

uint64_t getEpochMS() {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  const uint64_t millisecondsSinceEpoch =
      (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

  return millisecondsSinceEpoch;
}

#endif
