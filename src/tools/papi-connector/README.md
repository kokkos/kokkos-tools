## Using the PAPI Connector

To use the PAPI connector you have to compile it against the latest PAPI version that includes the new high-level API.

Details about PAPI's new high-level API can be found here:  
https://bitbucket.org/icl/papi/wiki/Home

#### Install PAPI (from version 6.0) or latest git version

```console
git clone https://bitbucket.org/icl/papi.git
```   
Installation instructions can be found here:  
https://icl.utk.edu/papi/software/index.html

#### Install the PAPI connector
```console
export PAPI_ROOT=<Path of PAPI>  
make
```
This will generate a dynamic library `kp_papi_connector.so`.

#### Run kokkos application with PAPI recording enabled
Before executing the Kokkos application you have to set the environment variable `KOKKOS_PROFILE_LIBRARY` to point to the dynamic library. Also add the library path of PAPI and PAPI connector to `LD_LIBRARY_PATH`.

```console
export KOKKOS_PROFILE_LIBRARY=kp_papi_connector.so
export LD_LIBRARY_PATH=<PAPI-LIB-PATH>:<PAPI-CONNECTOR-LIB-PATH>:$LD_LIBRARY_PATH
```

As implemented in PAPI’s high-level API, default PAPI events are measured if you have not specified any specific events. You can specify arbitrary events using the environment variable `PAPI_EVENTS`.

```console
export PAPI_EVENTS="PAPI_TOT_INS,PAPI_TOT_CYC"
```

Note that all environment variables that are used by PAPI can be set as well, see PAPI's documentation.

#### Analyze PAPI output
During the finalization phase of a Kokkos application, an output file is generated automatically. The output format is JSON and lists several PAPI events for each parallel construct or profile section. If not specified by the user, the output file will be located in the current working directory in the "papi" folder.

PAPI also offers a Python script which enhances the output
by creating some derived metrics, like IPC, MFlops/s, and MFlips/s as well as real and processor time in case the corresponding PAPI events have been recorded. 

```console
papi_hl_output_writer.py --source papi
```
