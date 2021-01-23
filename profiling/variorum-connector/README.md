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

This version of the connector was developed and tested on the Lassen cluster at LLNL.

There are 5 options available with this connector that are set with Enviornment variables as follows:
	1. VARIORUM_USING_MPI, this is set to either "true" or "false". The default is false. This option puts the connector in mpi mode where the output will be tied to an MPI rank 
	2. RANKED_OUTPUT, this option is set to either "true" or "false". The default option is false When set to true the ranked output will be put in separated designated files.
	3. KOKKOS_VARIORUM_FUNC_TYPE, this is set to either "json", "ppower" for print power or "both". This option is by default set to "both". There are two types of output this connector can provide, the power measurements printed out with their associated components or the direct jsoon data that holds the measrurements
	4. VERBOSE, this option is set to either "true" or "false" and is associated with the print power option. The verbose print power function will print out the node, cpu, gpu and memeory power. The non verbose print power option will just print out node pwoer. 
	5. VARIORUM_OUTPUT_PATH, this option enables the user to specify a desired output path. By default this option is set to "./" 
