# Kokkos Tools

Kokkos Tools provides a collection of lightweight profiling and debugging utilities that interface with instrumentation hooks in the Kokkos runtime. Unlike NVTX or ROCTx, Kokkos Tools emphasize Kokkos-centric analysis. Profiling hooks are included in Kokkos executables by default, so applications can load tools at runtime without recompilation.

**Kokkos Tools is part of the [Kokkos C++ Performance Portability Programming Ecosystem](https://kokkos.org).**
and meets the *maturing* life-cycle stage.


## Documentation

For complete documentation—including build instructions, usage, tool descriptions, tutorials, and contributing guidelines—see the [Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki).

## Obtaining Kokkos Tools

Development versions are available from the [`develop` branch](https://github.com/kokkos/kokkos-tools/tree/develop). Tagged releases appear on the [GitHub releases page](https://github.com/kokkos/kokkos-tools/releases).

```bash
git clone --branch develop https://github.com/kokkos/kokkos-tools.git
```

## Building

Building Kokkos Tools requires a C++20 compatible compiler or later.  CMake is the recommended build system:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=${YOUR_KOKKOS_TOOLS_INSTALL_DIR}
cmake --build build
cmake --install build
```

See the [wiki](https://github.com/kokkos/kokkos-tools/wiki) for Makefile builds, backend-specific connectors, and runtime configuration (`KOKKOS_TOOLS_LIBS`, `--kokkos-tools-libs`).

## Support

For questions, use Slack: https://kokkosteam.slack.com or open a [GitHub issue](https://github.com/kokkos/kokkos-tools/issues).

As a *maturing* subproject in the Kokkos Ecosystem, Kokkos Tools' testing regime is not as rigorous as we like it to be.
For details on the testing status of the 5.2 release please see [GitHub issue 338](https://github.com/kokkos/kokkos-tools/issues/338).

## Contributing

See the [wiki contributing guidance](https://github.com/kokkos/kokkos-tools/wiki#contributing) and [Submitting a Pull Request](https://github.com/kokkos/kokkos-tools/wiki/Submitting-a-Pull-Request).

## License

[![License](https://img.shields.io/badge/License-Apache--2.0_WITH_LLVM--exception-blue)](https://spdx.org/licenses/LLVM-exception.html)

The full license statement is available [here](https://kokkos.org/kokkos-core-wiki/license.html) or in [LICENSE](./LICENSE).
