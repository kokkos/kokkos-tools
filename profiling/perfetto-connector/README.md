# Perfetto Connector

This connector supports both the Perfetto inprocess backend, system backend, or both.
Connecting to Perfetto running with the system backend (i.e., as a daemon) is
recommended for MPI applications.

Visualization of the results can be performed by visiting
[ui.perfetto.dev](https://ui.perfetto.dev/) in a web browser and opening the trace file.

## Relevant Environment Variables

| Environment Variable                | Description                                       | Default Value              | Options              |
|-------------------------------------|---------------------------------------------------|----------------------------|----------------------|
| KOKKOSP_PERFETTO_SHMEM_SIZE_HINT_KB | Size of shared memory buffer in KB                | 10000 (10 MB)              |                      |
| KOKKOSP_PERFETTO_BUFFER_SIZE_KB     | Max size of the data collected                    | 1000000 (1 GB)             |                      |
| KOKKOSP_PERFETTO_FILL_POLICY        | Policy when the max buffer size is reached        | discard                    | discard, ring_buffer |
| KOKKOSP_PERFETTO_INPROCESS          | Generate output for data only within this process | 1                          | 0, 1                 |
| KOKKOSP_PERFETTO_SYSTEM_BACKEND     | Add data to existing perfetto daemon              | 0                          | 0, 1                 |
| KOKKOSP_PERFETTO_OUTPUT_FILE        | Output file for data collected in process         | "kokkosp-perfetto.pftrace" | any valid filepath   |

## Using Perfetto with System Backend

### Install Perfetto

[Please see Installation Instructions for Perfetto](https://perfetto.dev/docs/contributing/build-instructions).

### Start Traced and Perfetto Daemons

Enable `traced` and `perfetto` in the background:

```shell
traced --background
perfetto --out ./omnitrace-perfetto.proto --txt -c kokkos.cfg --background
```

The `kokkos.cfg` is provided as an example. It may be customized.
