This is a sampler utility that is intended to complement other tools in the Kokkos Tools set. This utility allows for sampling (rather than collecting) of profiling or debugging data gathered from a particular tool of the Kokkos Tools set. The Kokkos Tools user provides asampling rate via the environment variable KOKKOS_TOOLS_SAMPLER_SKIP.  

Tool fencing - which ensures state of sampled profiling and logging data in memory is captured at the time of the callback invocation - is important to control for performance of this utility while also ensuring accurate data. This utility handles this. 
However, turning on tool fencing for this to work is necessary for this to work. Turn on this fencing by setting the environment variable `KOKKOS_TOOLS_GLOBALFENCES` to be `1`. 
