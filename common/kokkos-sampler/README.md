This is a sampler utility that is intended to complement other tools in the Kokkos Tools set. This utility allows for sampling (rather than collecting) of profiling or debugging data gathered from a particular tool of the Kokkos Tools set.

To use this utility, a Kokkos Tools user provides a sampling probability by setting the environment variable `KOKKOS_TOOLS_SAMPLER_PROB` to a positive real number between 0.0 and 100.0. The user can alternatively set a sampling skip rate, i.e., the number of Kokkos kernel invocations to skip before the next sample is taken. The user does so by setting the environment variable `KOKKOS_TOOLS_SAMPLER_SKIP` to a non-negative integer.

If both sampling probability and sampling skip rate are set by the user, this sampling utility only uses the sampling probability for sampling; the utility sets the sampling skip rate to 1, incorporating no pre-defined periodicity in sampling. If neither sampling probability nor the sampling skip rate are set by the user, then randomized sampling is done, with the sampler's probability being 10.0 percent. The sampler is periodic only if the sampling probability is not set by the user and the sampling skip rate is set by the user.

For randomized sampling, the user can ensure reproducibility of this tool's output across multiple runs of a Kokkos application by setting `KOKKOS_TOOLS_RANDOM_SEED` to an integer value before all of the runs. If this environment variable is not set, the seed is based on the C time function.

In order for the state of the sampled profiling and logging data in memory to be captured at the time of the utility's callback invocation, it might be important to enforce fences. However, this also means that there are more synchronization points compared with running the program without the tool.
This fencing behavior can be controlled by setting the environment variable `KOKKOS_TOOLS_GLOBALFENCES`. A non-zero value implies global fences on invocation of the tool. The default is not to introduce extra fences.

