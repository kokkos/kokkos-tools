This combined tool system allows multiple profiling tools to work together as a single profiling library. This enables:

- Running multiple profiling strategies simultaneously
- Combining complementary analysis techniques
- Sharing a single interface with the Kokkos runtime

## Daemon-Style Tools

The kp_combined_daemon_timer.cpp demonstrates a background monitoring tool:

### Key Components:

- **Background Thread**:
   ```cpp
   static std::jthread s_timer_thread;
   ```
   Uses C++17's `std::jthread` to automatically join on destruction.

- **Thread Function**:
   ```cpp
   void timer_thread_func(std::stop_token stop_token) {
       while (!stop_token.stop_requested()) {
           // Record timestamp
           std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
       }
   }
   ```
   Periodically collects timestamps until stopped.

- **Kokkos Profiling Hooks**:
   - `kokkosp_init_library`: Starts the background thread
   - `kokkosp_finalize_library`: Stops the thread and outputs results

## Universal Tool Interface

The kp_universal.hpp provides a common interface for all tools:

```cpp
#define GENERATE_TOOL_HEADER(TOOL_NAME) \
namespace TOOL_NAME { \
    void kokkosp_init_library(...); \
    void kokkosp_finalize_library(); \
    // ... other function declarations
}
```

This macro generates consistent function declarations for each tool, allowing the combined template to access them uniformly.

## Creating a Combined Tool

The combined-tool-template.cpp shows how to create a tool that delegates to multiple implementation tools:

In this specific case, **Initialization/Finalization**:
   ```cpp
   void kokkosp_init_library(...) {
       // Initialize all component tools
       KokkosTools::CombinedSimple::kokkosp_init_library(...);
       KokkosTools::CombinedDaemon::kokkosp_init_library(...);
   }
   ```

Along with other profiling hooks, this can be used to propagate events to all included tools.

## Building Combined Tools

The CMake system builds the combined tool as follows:

```cmake
# Build individual tools
add_subdirectory(example-tools)

# Create combined tool
kp_add_library(kp_combined_tool 
    combined-tool-template.cpp
)

# Link individual tools
target_link_libraries(kp_combined_tool PUBLIC 
    kp_combined_simple 
    kp_combined_daemon_timer
)
```

## Adding a New Tool

To add a new tool to the combined system:

* Create your tool implementation with required profiling hooks
* Add a namespace declaration in kp_universal.hpp
* Add initialization and finalization calls in combined-tool-template.cpp
* Add your tool library to `target_link_libraries` in CMakeLists.txt

## Usage

Load the combined tool with Kokkos by setting the appropriate environment variable:

```bash
export KOKKOS_PROFILE_LIBRARY=/path/to/libkp_combined_tool.so
```

When your application runs, all included tools will activate simultaneously.