Kokkos Variorum Connector

Authors: Zachary S. Frye, David Poliakoff
Organization: CASC at LLNL & Sandia National Laboratory 

This is the development branch of a Kokkos Variorum connector. 

Variorum: https://github.com/LLNL/variorum

Kokkos: https://github.com/kokkos/kokkos


The connector source code is named variorum-connector.cpp, and can be compiled simply using make. The connector currently officially supports CPU applications. 

$(VARIORUM_BUILD_ROOT) shuold point to an installed Variorum build.

    export VARIORUM_ROOT=/path/to/variorum/build 

After compiling the .so file, and installing Variorum, using the tool can be done with the following steps:


1. Set the required enviornment variables: 
    export LD_LIBRARY_PATH=$(VARIORUM_ROOT)/lib & 
    export KOKKOS_PROFILE_LIBRARY=$(VARIORUM_CONNECTOR_PATH)/variorum_connector.so
2. Run a kokkos exectuable

example output from an Intel cluster:

    Start Time: 9999999999
    _LOG_VARIORUM_ENTER:$(VARIORUM_ROOT)/variorum.c:variorum_print_power::552
    Intel Model: 0x4f
    Running fm_06_4f_get_power
    _PACKAGE_ENERGY_STATUS Offset Host Socket Bits Joules Watts Elapsed Timestamp
    _PACKAGE_ENERGY_STATUS 0x611 quartz32 0 0x5bfe1843 94200.379089 0.000000 0.000000 0.000035
    _PACKAGE_ENERGY_STATUS 0x611 quartz32 1 0x1b3360d2 27853.512817 0.000000 0.000000 0.000035
    _DRAM_ENERGY_STATUS Offset Host Socket Bits Joules Watts Elapsed Timestamp
    _DRAM_ENERGY_STATUS 0x619 quartz32 0 0x8138efde 33080.936981 0.000000 0.000000 0.000035
    _DRAM_ENERGY_STATUS 0x619 quartz32 1 0xf1c1fb81 61889.982437 0.000000 0.000000 0.000035
    _LOG_VARIORUM_EXIT:$(VARIORUM_ROOT)/variorum.c:variorum_print_power::569
    _LOG_VARIORUM_ENTER:$(VARIORUM_ROOT)/variorum.c:variorum_print_power::552

This connector has been developed and tested on Intel Broadwell and IBM Power9 clusters at LLNL. 
