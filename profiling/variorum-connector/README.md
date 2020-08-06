Kokkos Variorum Connector

Authors: Zachary S. Frye, David Poliakoff
Organization: CASC at LLNL & Sandia National Laboratory 

This is the development branch of a kokkos/variorum connector. 

Variorum: https://github.com/LLNL/variorum

Kokkos: https://github.com/kokkos/kokkos


The connector source code named variorum-connector.cpp, and can be compiled simply using make. The makefile's variorum information points to where i have my variorum installed on my system, the connector requires a build of variorum and a change to these paths in order for this connector to compile and work for you. 

You will also have to set the LD_LIBRARY_PATH enviornment variable to the where the /lib directory is in your variorum build directory

After compiling the .so file, you can have kokkos use it for profiling by setting this option:

KOKKOS_PROFILE_LIBRARY=/path/to/where/your/library/lives.so 

This connector has been developed and testd on the quartz (intel xeon) and lassen (IBM Power9) clusters at LLNL. 

Power caps can be set on intel CPU architectures and power9 nodes using <variorum buidl direcotry here>/examples/variorum-set-and-verify-node-power-limit-example <power limit value in Watts>