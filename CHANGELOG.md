# Changelog

## 5.3.0

[Full Changelog](https://github.com/kokkos/kokkos-tools/compare/5.2.0...5.3.0)

### Profiling interface
- Update the Kokkos Profiling interface to reflect the changes in Kokkos Core, including support for `Kokkos::single` and the NextSilicon backend (#358).

### Tools
- mem-usage: added a missing `<string>` include (#353).
- Added support for `Kokkos::single` to the kernel-filter, kokkos-sampler, kernel-logger and simple-kernel-timer tools (#331).
