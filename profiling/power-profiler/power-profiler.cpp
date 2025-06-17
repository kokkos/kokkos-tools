//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

/**
 * Energy Consumption Toolbox using Variorum
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <stack>
#include <cstdlib>

#include "kp_core.hpp"

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

namespace KokkosTools {
namespace PowerProfiler {

// --- Data Structures for Logging ---

// Enum to define kernel types
enum class KernelType {
    FOR,
    SCAN,
    REDUCE,
    UNKNOWN
};

// Helper function to convert KernelType enum to string
std::string kernel_type_to_string(KernelType type) {
    switch (type) {
        case KernelType::FOR:    return "For";
        case KernelType::SCAN:   return "Scan";
        case KernelType::REDUCE: return "Reduce";
        case KernelType::UNKNOWN: return "Unknown";
    }
    return "Unknown"; // Should not be reached
}

// Structure to hold a single power reading
struct PowerReading {
    long long timestamp_ms; // Epoch timestamp in milliseconds
    std::map<uint32_t, double> gpu_powers; // Map of GPU ID to power in Watts
};

// Structure to hold kernel execution data
struct KernelTiming {
    std::string name;
    KernelType type; // Added to store the kernel type
    long long start_time_ms; // Epoch timestamp in milliseconds
    long long end_time_ms;   // Epoch timestamp in milliseconds
};

// Global lists to store recorded data
static std::vector<PowerReading> s_power_readings_log;
static std::vector<KernelTiming> s_kernel_timings_log;

// Mutexes to protect access to global logs from multiple threads
static std::mutex s_power_log_mutex;
static std::mutex s_kernel_log_mutex;

// Map to temporarily store kernel names and their start times for 'end' events
static std::unordered_map<uint64_t, KernelTiming> s_active_kernels;
static std::mutex s_active_kernels_mutex; // Mutex for s_active_kernels

// Stack to manage nested profiling regions (LIFO) //TODO: Mark regions on kernel data for plotting
static std::vector<std::string> s_profile_regions_stack; // Use std::vector as a stack
static std::mutex s_profile_regions_mutex; // Mutex for the regions stack


// --- Helper for JSON parsing with smart pointer ---
struct JsonDeleter {
    void operator()(json_t* json) const {
        if (json) {
            json_decref(json);
        }
    }
};
using unique_json_ptr = std::unique_ptr<json_t, JsonDeleter>;


// --- Custom Deleter for char* allocated by C functions (like malloc/strdup) ---
struct CFreeDeleter {
    void operator()(char* ptr) const {
        if (ptr) {
            free(ptr); // Use free() as variorum_get_power_json likely allocates this way
        }
    }
};

// Alias type for a unique_ptr that manages C strings allocated with free()
using unique_c_string_ptr = std::unique_ptr<char, CFreeDeleter>;


/**
 * @brief Retrieves and parses JSON data from Variorum.
 * @return unique_json_ptr A smart pointer to the root JSON object, or nullptr on failure.
 */
static unique_json_ptr getVariorumJsonData() {
    char* json_string_c_raw = nullptr; // Raw pointer for C API output
    if (variorum_get_power_json(&json_string_c_raw) != 0) {
        // std::cerr << "PowerProfiler: variorum_get_power_json() failed." << std::endl;
        return nullptr;
    }

    unique_c_string_ptr json_string_c(json_string_c_raw);

    if (!json_string_c) {
        // std::cerr << "PowerProfiler: variorum_get_power_json() returned success but a null pointer, indicating no data." << std::endl;
        return nullptr;
    }
    std::string json_string(json_string_c.get());

    json_error_t error;
    json_t* root_ptr = json_loads(json_string.c_str(), 0, &error);

    if (!root_ptr) {
        // std::cerr << "PowerProfiler: Failed to parse JSON: " << error.text << std::endl;
        return nullptr;
    }
    return unique_json_ptr(root_ptr);
}

/**
 * @brief Scans Variorum data to find all existing GPU device IDs.
 * @return std::vector<uint32_t> A vector of found GPU IDs.
 */
std::vector<uint32_t> scanForAvailableGpuDevices() {
    std::set<uint32_t> found_device_ids;
    unique_json_ptr root = getVariorumJsonData();

    if (!root) {
        return {}; 
    }

    json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
    if (!host_obj) {
        std::cerr << "PowerProfiler: No hostname object found in JSON." << std::endl;
        return {};
    }

    //! Assumption that socket_0 is the main concern
    json_t* socket_0 = json_object_get(host_obj, "socket_0");
    if (!socket_0 || !json_is_object(socket_0)) {
        std::cerr << "PowerProfiler: 'socket_0' object not found or invalid." << std::endl;
        return {};
    }

    json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
    if (!power_gpu_watts || !json_is_object(power_gpu_watts)) {
        std::cerr << "PowerProfiler: 'power_gpu_watts' object not found or invalid." << std::endl;
        return {};
    }

    const char* key;
    json_t* value;
    json_object_foreach(power_gpu_watts, key, value) {
        std::string s_key(key);
        if (s_key.starts_with("GPU_")) { 
            try {
                uint32_t device_id = std::stoul(s_key.substr(4));
                found_device_ids.insert(device_id);
            } catch (const std::invalid_argument& e) {
                std::cerr << "PowerProfiler: Could not parse GPU ID from key: " << s_key << " (" << e.what() << ")" << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "PowerProfiler: GPU ID out of range from key: " << s_key << " (" << e.what() << ")" << std::endl;
            }
        }
    }

    return std::vector<uint32_t>(found_device_ids.begin(), found_device_ids.end());
}

/**
 * @brief Retrieves the current power for a specific list of GPU devices.
 * @param device_ids A vector of GPU device IDs for which to retrieve power.
 * @return std::map<uint32_t, double> A map of device IDs to their respective power.
 */
std::map<uint32_t, double> getCurrentPowerForDevices(const std::vector<uint32_t>& device_ids) {
    std::map<uint32_t, double> power_readings;
    unique_json_ptr root = getVariorumJsonData();

    if (!root) {
        return {}; 
    }

    json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
    if (!host_obj) {
        return {};
    }

    json_t* socket_0 = json_object_get(host_obj, "socket_0");
    if (!socket_0 || !json_is_object(socket_0)) {
        return {};
    }

    json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
    if (!power_gpu_watts || !json_is_object(power_gpu_watts)) {
        return {};
    }

    for (uint32_t device_id : device_ids) {
        std::string gpu_key = "GPU_" + std::to_string(device_id);
        json_t* power_value = json_object_get(power_gpu_watts, gpu_key.c_str());

        if (json_is_number(power_value)) {
            power_readings[device_id] = json_number_value(power_value);
        }
    }

    return power_readings;
}

// Global variable to store available GPU devices discovered at initialization
static std::vector<uint32_t> s_available_gpu_devices;

// Global std::jthread for continuous power monitoring (C++20)
static std::jthread s_power_monitoring_thread;
static const int MONITOR_INTERVAL_MS = 20; // 20 milliseconds = 50 samples/second : Hardware limitation depending on monitored device

/**
 * @brief Function executed by the power monitoring thread.
 * It periodically collects GPU power and logs it.
 * @param stop_token Token to request the thread to stop.
 */
void power_monitoring_thread_func(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        std::map<uint32_t, double> current_powers = getCurrentPowerForDevices(s_available_gpu_devices);

        if (!current_powers.empty()) {
            PowerReading pr;
            pr.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch()).count();
            pr.gpu_powers = current_powers;
            
            std::lock_guard<std::mutex> lock(s_power_log_mutex);
            s_power_readings_log.push_back(pr);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));
    }
    std::cout << "PowerProfiler: Power monitoring thread stopped." << std::endl;
}

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
    printf("-----------------------------------------------------------\n");
    printf("KokkosP: Power Profiler (sequence is %d, version: %lu)\n",
         loadSeq, interfaceVer);
    printf("-----------------------------------------------------------\n");

    s_available_gpu_devices = scanForAvailableGpuDevices();
    if (s_available_gpu_devices.empty()) {
        std::cerr << "PowerProfiler: No GPU devices found during initialization. Power monitoring will not start." << std::endl;
    } else {
        std::cout << "PowerProfiler: Found " << s_available_gpu_devices.size() << " GPU(s):";
        for (uint32_t id : s_available_gpu_devices) {
            std::cout << " GPU_" << id;
        }
        std::cout << std::endl;

        s_power_monitoring_thread = std::jthread(power_monitoring_thread_func);
        std::cout << "PowerProfiler: Started background power monitoring thread." << std::endl;
    }
}

void kokkosp_finalize_library() {
    if (s_power_monitoring_thread.joinable()) {
        s_power_monitoring_thread.request_stop();
        std::cout << "PowerProfiler: Requested power monitoring thread to stop. Waiting for it to finish..." << std::endl;
    } else {
        std::cout << "PowerProfiler: Power monitoring thread was not running or not joinable." << std::endl;
    }

    // Print all collected power readings in a parsable format
    // Detail of the format: Timestamp, GPU ID, Power
    std::cout << "\n--- POWER_DATA_START ---" << std::endl;
    std::lock_guard<std::mutex> power_lock(s_power_log_mutex);
    if (s_power_readings_log.empty()) {
        std::cout << "POWER_READING_NONE" << std::endl;
    } else {
        for (const auto& reading : s_power_readings_log) {
            for (const auto& pair : reading.gpu_powers) {
                std::cout << "POWER_READING," << reading.timestamp_ms << ","
                          << pair.first << "," << pair.second << std::endl;
            }
        }
    }
    std::cout << "--- POWER_DATA_END ---" << std::endl;

    // Print all collected kernel timings in a parsable format
    // Detail of the format: Timestamp, Kernel Type, Name, Start Time, End Time, Duration
    std::cout << "\n--- KERNEL_DATA_START ---" << std::endl;
    std::lock_guard<std::mutex> kernel_log_lock(s_kernel_log_mutex);
    if (s_kernel_timings_log.empty()) {
        std::cout << "KERNEL_TIMING_NONE" << std::endl;
    } else {
        for (const auto& timing : s_kernel_timings_log) {
            std::cout << "KERNEL_TIMING," << kernel_type_to_string(timing.type) << ",\""
                      << timing.name << "\"," << timing.start_time_ms << ","
                      << timing.end_time_ms << ","
                      << (timing.end_time_ms - timing.start_time_ms) << std::endl;
        }
    }
    std::cout << "--- KERNEL_DATA_END ---" << std::endl;

    printf("-----------------------------------------------------------\n");
    printf("KokkosP: Finalization of Power Profiler. Complete.\n");
    printf("-----------------------------------------------------------\n");
}

std::string device_type_to_string(
    const Kokkos::Tools::Experimental::DeviceType& deviceType) {
    switch (deviceType) {
        case Kokkos::Tools::Experimental::DeviceType::Serial:
        return "SERIAL";
        case Kokkos::Tools::Experimental::DeviceType::OpenMP:
        return "OPENMP";
        case Kokkos::Tools::Experimental::DeviceType::Cuda: return "CUDA";
        case Kokkos::Tools::Experimental::DeviceType::HIP: return "HIP";
        case Kokkos::Tools::Experimental::DeviceType::OpenMPTarget:
        return "OPENMPTARGET";
        case Kokkos::Tools::Experimental::DeviceType::HPX: return "HPX";
        case Kokkos::Tools::Experimental::DeviceType::Threads:
        return "THREADS";
        case Kokkos::Tools::Experimental::DeviceType::SYCL: return "SYCL";
        case Kokkos::Tools::Experimental::DeviceType::OpenACC:
        return "OPENACC";
        default: return "UNKNOWN";
    }
}

// --- Kernels Launch/End ---

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    // Store kernel start time and name
    if (kID) {
        std::lock_guard<std::mutex> lock(s_active_kernels_mutex);
        s_active_kernels[*kID] = {std::string(name), KernelType::FOR, current_time_ms, 0}; // Set type to FOR
    }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    std::lock_guard<std::mutex> active_kernels_lock(s_active_kernels_mutex);
    auto it = s_active_kernels.find(kID);
    if (it != s_active_kernels.end()) {
        KernelTiming kt = it->second; // Get the stored timing info
        kt.end_time_ms = current_time_ms; // Update end time

        // Add to the final kernel timings log
        std::lock_guard<std::mutex> kernel_log_lock(s_kernel_log_mutex);
        s_kernel_timings_log.push_back(kt);
        
        s_active_kernels.erase(it); // Clean up active kernels map
    }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    if (kID) {
        std::lock_guard<std::mutex> lock(s_active_kernels_mutex);
        s_active_kernels[*kID] = {std::string(name), KernelType::SCAN, current_time_ms, 0}; // Set type to SCAN
    }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    std::lock_guard<std::mutex> active_kernels_lock(s_active_kernels_mutex);
    auto it = s_active_kernels.find(kID);
    if (it != s_active_kernels.end()) {
        KernelTiming kt = it->second;
        kt.end_time_ms = current_time_ms;

        std::lock_guard<std::mutex> kernel_log_lock(s_kernel_log_mutex);
        s_kernel_timings_log.push_back(kt);

        s_active_kernels.erase(it);
    }
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    if (kID) {
        std::lock_guard<std::mutex> lock(s_active_kernels_mutex);
        s_active_kernels[*kID] = {std::string(name), KernelType::REDUCE, current_time_ms, 0}; // Set type to REDUCE
    }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
    long long current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch()).count();
    std::lock_guard<std::mutex> active_kernels_lock(s_active_kernels_mutex);
    auto it = s_active_kernels.find(kID);
    if (it != s_active_kernels.end()) {
        KernelTiming kt = it->second;
        kt.end_time_ms = current_time_ms;

        std::lock_guard<std::mutex> kernel_log_lock(s_kernel_log_mutex);
        s_kernel_timings_log.push_back(kt);

        s_active_kernels.erase(it);
    }
}

void kokkosp_push_profile_region(char const* regionName) {
    std::lock_guard<std::mutex> lock(s_profile_regions_mutex);
    s_profile_regions_stack.push_back(std::string(regionName));
    // Print for immediate feedback during execution
    printf("KokkosP: Entering profiling region: %s\n", regionName);
}

void kokkosp_pop_profile_region() {
    std::lock_guard<std::mutex> lock(s_profile_regions_mutex);
    if (!s_profile_regions_stack.empty()) {
        printf("KokkosP: Exiting profiling region: %s\n", s_profile_regions_stack.back().c_str());
        s_profile_regions_stack.pop_back();
    } else {
        printf("KokkosP: Warning: Attempted to pop a profiling region from an empty stack.\n");
    }
}


// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0,
            sizeof(my_event_set));  // zero any pointers not set here
    my_event_set.init                  = kokkosp_init_library;
    my_event_set.finalize              = kokkosp_finalize_library;
    my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
    my_event_set.push_region           = kokkosp_push_profile_region;
    my_event_set.pop_region            = kokkosp_pop_profile_region;
    return my_event_set;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::PowerProfiler;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)

}