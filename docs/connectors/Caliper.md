# Caliper Connector

## Overview

[Caliper](https://software.llnl.gov/Caliper/) is a program instrumentation and performance measurement framework developed at Lawrence Livermore National Laboratory. It provides flexible and composable performance measurement capabilities for HPC applications.

Caliper has built-in Kokkos support and provides extensive capabilities for collecting, aggregating, and analyzing performance data from Kokkos applications.

## Key Features

- **Built-in Kokkos Support**: Native integration when configured with `WITH_KOKKOS=ON`
- **Flexible Configuration**: Composable measurement configurations through ConfigManager
- **Multiple Output Formats**: Support for various output formats and aggregation strategies
- **MPI Integration**: Built-in support for MPI applications with cross-process aggregation
- **Low Overhead**: Designed for production-level performance measurement

## Installation and Setup

### Building Caliper with Kokkos Support

Caliper must be configured with Kokkos support enabled:

```bash
git clone https://github.com/LLNL/Caliper.git
cd Caliper
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install -DWITH_KOKKOS=ON ..
make
make install
```

For detailed build instructions, see the [Caliper documentation](https://software.llnl.gov/Caliper/build.html).

### Using the Connector

Set the `KOKKOS_PROFILE_LIBRARY` environment variable to point to the Caliper library:

```bash
# Linux
export KOKKOS_PROFILE_LIBRARY=/path/to/libcaliper.so

# macOS
export KOKKOS_PROFILE_LIBRARY=/path/to/libcaliper.dylib

# Run your application
./your_kokkos_application
```

## Configuration

Caliper provides extensive configuration options through the `CALI_CONFIG` environment variable and its ConfigManager.

### Basic Profiling

For basic runtime reporting:

```bash
export CALI_CONFIG=runtime-report
./your_application
```

### Kokkos-Specific Configuration

To integrate Kokkos profiling data, add `profile.kokkos` to your configuration:

```bash
# Runtime report with Kokkos profiling
export CALI_CONFIG=runtime-report(profile.kokkos)

# Runtime report with MPI and Kokkos profiling
export CALI_CONFIG=runtime-report(profile.mpi,profile.kokkos)
```

### Common Configurations

| Configuration | Description |
|---------------|-------------|
| `runtime-report(profile.kokkos)` | Basic runtime statistics for Kokkos kernels |
| `hatchet-region-profile(profile.kokkos)` | Hierarchical profile in Hatchet format |
| `event-trace(profile.kokkos)` | Detailed event trace |
| `spot(profile.kokkos)` | Comprehensive performance analysis |

### Advanced Configuration

Caliper's ConfigManager accepts various parameters for fine-grained control:

```bash
# Custom output file
export CALI_CONFIG=runtime-report(profile.kokkos,output=my_profile.json)

# Multiple metrics
export CALI_CONFIG=runtime-report(profile.kokkos,calc.inclusive,calc.exclusive)

# Filtering
export CALI_CONFIG=runtime-report(profile.kokkos,region.filter=myregion)
```

## Output

Caliper can produce output in several formats:
- **Console**: Human-readable runtime reports
- **JSON**: Machine-readable data for post-processing
- **Hatchet**: For use with the Hatchet analysis framework
- **Recorder**: Binary trace format for detailed analysis

Output location and format are controlled through configuration options.

## Integration with Analysis Tools

### Hatchet

Caliper integrates with [Hatchet](https://github.com/LLNL/hatchet), a Python-based tool for analyzing hierarchical performance data:

```bash
export CALI_CONFIG=hatchet-region-profile(profile.kokkos,output=profile.json)
./your_application
```

Then analyze in Python:
```python
import hatchet as ht
gf = ht.GraphFrame.from_caliper("profile.json")
```

## Resources and Documentation

### Official Caliper Resources

- **Website**: [software.llnl.gov/Caliper](https://software.llnl.gov/Caliper/)
- **Documentation**: [Caliper Documentation](https://software.llnl.gov/Caliper/)
- **Repository**: [GitHub - LLNL/Caliper](https://github.com/LLNL/Caliper)
- **ConfigManager Guide**: [Configuration Guide](https://software.llnl.gov/Caliper/ConfigManagerAPI.html)

### Kokkos-Specific Resources

- **Local README**: [Caliper Connector README](../../profiling/caliper-connector/README.md)
- **Kokkos Tools Wiki**: [Main Wiki](https://github.com/kokkos/kokkos-tools/wiki)

## Example Workflow

A typical workflow for profiling a Kokkos application with Caliper:

1. **Build Caliper with Kokkos support**
2. **Configure your measurement**:
   ```bash
   export KOKKOS_PROFILE_LIBRARY=/path/to/libcaliper.so
   export CALI_CONFIG=runtime-report(profile.kokkos,output=profile.json)
   ```
3. **Run your application**:
   ```bash
   ./your_application
   ```
4. **Analyze results**:
   - View console output
   - Post-process JSON output
   - Use Hatchet for detailed analysis

## Advanced Usage

### Custom Annotations

In addition to automatic Kokkos kernel profiling, you can add custom annotations:

```cpp
#include <caliper/cali.h>

void my_function() {
    CALI_MARK_FUNCTION_BEGIN;
    // Your code
    CALI_MARK_FUNCTION_END;
}
```

### Sampling

Caliper supports sampling-based profiling for lower overhead:

```bash
export CALI_CONFIG=sample-report(profile.kokkos)
```

## Troubleshooting

### Issue: Caliper library not found

**Solution**: Ensure the Caliper library path is in your `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (macOS):
```bash
export LD_LIBRARY_PATH=/path/to/caliper/lib:$LD_LIBRARY_PATH
```

### Issue: No Kokkos data in output

**Solution**: Verify that:
1. Caliper was built with `WITH_KOKKOS=ON`
2. Your configuration includes `profile.kokkos`
3. Your Kokkos application has profiling enabled (`Kokkos_ENABLE_LIBDL=ON`)

## See Also

- [ScoreP Connector](ScoreP.md) - Performance measurement infrastructure with tracing
- [Timemory Connector](Timemory.md) - Modular performance analysis framework
- [Kokkos Tools Wiki](https://github.com/kokkos/kokkos-tools/wiki)
- [Main Kokkos Tools Documentation](../README.md)
