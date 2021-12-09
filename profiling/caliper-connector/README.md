# Caliper Connector

Caliper has built-in Kokkos support if configured with `WITH_KOKKOS=ON`.
Set `KOKKOS_PROFILE_LIBRARY=/path/to/libcaliper.<ext>` in the environment where `<ext>` is
either `so` (Linux) or `dylib` (macOS).

Note that Caliper has its own extensive documentation [here](https://software.llnl.gov/Caliper/index.html). In general, you can add Kokkos functionality to the recommendations there by passing `profile.kokkos` to their ConfigManager via the `CALI_CONFIG` environment variable. For example, Caliper recommends you set that to `runtime-report(profile.mpi)` to get a report of runtime with MPI aggregation. Simply change that to `runtime-report(profile.mpi,profile.kokkos)` and you get Kokkos timers integrated.
