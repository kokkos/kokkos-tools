# Copilot Instructions for kokkos-tools

These notes teach AI coding agents how to be productive in this repo quickly: where things live, how they build, and the key patterns for integrating new tools or connectors.

## Big picture

- Purpose: A suite of lightweight profiling/debugging tools for Kokkos applications, loaded via Kokkos profiling hooks. Agents typically implement or modify a tool (profiler/connector) as a shared library and run it by setting `KOKKOS_TOOLS_LIBS`.
- Architecture:
  - Root CMake (`CMakeLists.txt`) orchestrates optional subprojects under `common/`, `debugging/`, and `profiling/`.
  - Each tool is an independent shared lib built from its subdirectory via CMake or local Makefiles.
  - Optional monolithic "single" library (`profiling/all/`) can dispatch multiple profilers at runtime when `KokkosTools_ENABLE_SINGLE=ON`.
  - Common headers are generated to `bld/common/kp_config.hpp` from `common/kp_config.hpp.in` and included via `COMMON_HEADERS_PATH`.
  - External integrations (PAPI, VTune, NVTX, ROCTX, Caliper, Variorum, MPI) are conditionally discovered/added by CMake.

## Key directories

- `common/` — shared utilities and helper tools
  - `kernel-filter/` and `kokkos-sampler/` are opt-in utilities that gate or sample profiling.
- `debugging/kernel-logger/` — prints Kokkos kernel/region events.
- `profiling/` — the main profiler/connectors collection (memory tools, tracing, GPU/third-party connectors).
- `example/` — small example using the monolithic interface when `KokkosTools_ENABLE_SINGLE=ON`.
- `tests/` — gtest-based tests, gated by `KokkosTools_ENABLE_TESTS`.
- `cmake/` — helper modules (Kokkos config discovery, TPLs, Apex/git submodule, utilities).

## Build workflows

- CMake (recommended for full suite):
  - From a fresh build dir: `cmake .. [-DKokkosTools_ENABLE_*=ON/OFF]` then `make` and optionally `make install`.
  - Kokkos detection and reuse: `-DKokkosTools_REUSE_KOKKOS_COMPILER=ON` reuses Kokkos' compiler/standard. Requires Kokkos to be discoverable.
  - Toggle profilers/connectors by options in `CMakeLists.txt`:
    - `KokkosTools_ENABLE_PAPI`, `KokkosTools_ENABLE_MPI`, `KokkosTools_ENABLE_CALIPER`, `KokkosTools_ENABLE_APEX`, `KokkosTools_ENABLE_SINGLE`, `KokkosTools_ENABLE_EXAMPLES`, `KokkosTools_ENABLE_TESTS`.
  - macOS specific: shared libs are `.dylib`. On Linux they are `.so`.
- Makefile (fast per-tool build): `cd <tool dir> && make` builds the local shared lib.
- Generated headers: `common/kp_config.hpp` is configured during CMake; avoid committing generated files.

## Running tools

- Set env var with one or more libraries:
  - `KOKKOS_TOOLS_LIBS=/path/to/lib<tool>.so` (Linux) or `.dylib` (macOS).
  - Some docs refer to `KOKKOS_TOOLS_LIBRARY`; this repo uses `KOKKOS_TOOLS_LIBS` in README. Keep consistency in new docs and examples.
- Ensure your Kokkos app is built with `Kokkos_ENABLE_LIBDL=ON` (typical default) so hooks load dynamically.

## Patterns when adding a new tool

- Subdirectory layout mirrors existing tools:
  - Create `profiling/<my-tool>/` (or `debugging/<...>/`) with `CMakeLists.txt` and source files.
  - Export target with `install(TARGETS ...)` so users can `make install`.
  - Include `common` and `profiling/all` headers as needed; most tools include `profiling/all/kp_core.hpp` or related hook headers.
- Hook usage:
  - Implement Kokkos profiling callbacks (e.g., begin/end kernel, memory alloc/free) in your lib and register on load.
  - Use labels provided by Kokkos constructs for kernel-centric analysis; follow examples like `simple-kernel-timer`, `memory-usage`, `kernel-logger`.
- Conditional availability:
  - Gate platform-specific integrations (NVTX on CUDA, ROCTX on HIP) with CMake `if(Kokkos_ENABLE_CUDA)` / `if(Kokkos_ENABLE_HIP)`.
  - Third-party connectors should `find_package(...)` or rely on env variables (e.g., `VTUNE_HOME`).

## Conventions and gotchas

- No in-source builds: CMake hard-errors if `CMAKE_SOURCE_DIR == CMAKE_BINARY_DIR`.
- Respect `BUILD_SHARED_LIBS` and Apple/Windows switches in root CMake.
- Monolithic vs. per-tool:
  - Monolithic single-lib (`profiling/all/`) requires `KokkosTools_ENABLE_SINGLE=ON`; examples and some tests assume this.
- MPI support toggled via `KokkosTools_ENABLE_MPI`; memory-hwm-mpi is conditionally built.
- Some connectors (systemtap) require `dtrace` (Unix-only) and are skipped if not present.

## Examples in repo

- `debugging/kernel-logger/kp_kernel_logger.cpp` — demonstrates region and kernel start/stop callbacks.
- `profiling/simple-kernel-timer/` — minimal timing of kernel events.
- `common/kernel-filter/` — shows how to gate tools to specific kernels.

## Testing & CI hooks

- Tests: enable via `-DKokkosTools_ENABLE_TESTS=ON`; gtest setup is under `cmake/BuildGTest.cmake` and `tests/`.
- Examples: enable via `-DKokkosTools_ENABLE_EXAMPLES=ON`; note warning if single-lib is disabled.

## Debugging tips

- Verify the tool is loaded: print a log on library init; check `KOKKOS_TOOLS_LIBS` path and file extension.
- On macOS, prefer absolute paths for `.dylib` in `KOKKOS_TOOLS_LIBS`.
- Cross-check that Kokkos app has profiling hooks enabled and `LIBDL` support.

---

Questions or unclear sections? Tell us which workflows or directories you’d like more detail on (e.g., perfetto/chrome tracing setup, PAPI connector versions, or single-library dispatch), and we’ll refine these instructions.