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

#include "kp_nvml_energy_profiler.hpp"
#include <cstdio>
#include <unistd.h>
#include <inttypes.h>

namespace KokkosTools {
namespace NVMLEnergyProfiler {

DataManager* g_data_manager = nullptr;

DataManager::DataManager() : nvml_initialized(false) {}

DataManager::~DataManager() {
    finalize();
}

bool DataManager::initialize() {
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        printf("KokkosP NVML Energy: Failed to initialize NVML: %s\n", nvmlErrorString(result));
        return false;
    }
    
    // Get the first GPU device
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        printf("KokkosP NVML Energy: Failed to get device handle: %s\n", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }
    
    // Test if energy monitoring is available
    unsigned long long energy;
    result = nvmlDeviceGetTotalEnergyConsumption(device, &energy);
    if (result != NVML_SUCCESS) {
        printf("KokkosP NVML Energy: Energy monitoring not available: %s\n", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }
    
    nvml_initialized = true;
    printf("KokkosP NVML Energy: Initialized successfully\n");
    return true;
}

void DataManager::finalize() {
    if (nvml_initialized) {
        nvmlShutdown();
        nvml_initialized = false;
    }
}

unsigned long long DataManager::get_current_energy_mj() const {
    if (!nvml_initialized) return 0;
    
    unsigned long long energy;
    nvmlReturn_t result = nvmlDeviceGetTotalEnergyConsumption(device, &energy);
    if (result != NVML_SUCCESS) {
        printf("KokkosP NVML Energy: Failed to get energy consumption: %s\n", nvmlErrorString(result));
        return 0;
    }
    return energy;
}

void DataManager::start_region(const std::string& name, RegionType type) {
    TimingEnergyInfo region;
    region.name = name;
    region.type = type;
    region.start_time = std::chrono::high_resolution_clock::now();
    region.start_energy_mj = get_current_energy_mj();
    active_regions.push_back(region);
}

void DataManager::end_region() {
    if (!active_regions.empty()) {
        auto& region = active_regions.back();
        region.end_time = std::chrono::high_resolution_clock::now();
        region.end_energy_mj = get_current_energy_mj();
        region.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            region.end_time - region.start_time);
        region.delta_energy_mj = region.end_energy_mj - region.start_energy_mj;
        
        // Power (W) = (delta_energy_mj / 1000) [Joules] / (duration_ns / 1e9) [seconds]
        // => Power = (delta_energy_mj * 1e6) / duration_ns
        if (region.duration.count() > 0) {
            region.average_power_w = (static_cast<double>(region.delta_energy_mj) * 1e6) / 
                                   static_cast<double>(region.duration.count());
        } else {
            region.average_power_w = 0.0;
        }
        
        if (region.type == RegionType::UserRegion) {
            completed_regions.push_back(region);
        } else {
            completed_kernels.push_back(region);
        }
        active_regions.pop_back();
    }
}

const char* DataManager::region_type_to_string(RegionType type) const {
    switch (type) {
        case RegionType::ParallelFor: return "parallel_for";
        case RegionType::ParallelReduce: return "parallel_reduce";
        case RegionType::ParallelScan: return "parallel_scan";
        case RegionType::UserRegion: return "user_region";
        default: return "unknown";
    }
}

void DataManager::write_kernel_data(const std::string& filename) const {
    if (completed_kernels.empty()) return;

    FILE* kernels_file = fopen(filename.c_str(), "w");
    if (kernels_file) {
        fprintf(kernels_file, "name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns,start_energy_mj,end_energy_mj,delta_energy_mj,average_power_w\n");
        for (const auto& kernel : completed_kernels) {
            auto start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                kernel.start_time.time_since_epoch())
                                .count();
            auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            kernel.end_time.time_since_epoch())
                            .count();
            fprintf(kernels_file, "%s,%s,%" PRId64 ",%" PRId64 ",%" PRId64 ",%llu,%llu,%llu,%.6f\n",
                    kernel.name.c_str(), 
                    region_type_to_string(kernel.type), 
                    start_ns,
                    end_ns, 
                    (int64_t)kernel.duration.count(),
                    kernel.start_energy_mj,
                    kernel.end_energy_mj,
                    kernel.delta_energy_mj,
                    kernel.average_power_w);
        }
        fclose(kernels_file);
        char cwd[256];
        if (getcwd(cwd, 256) != nullptr) {
            printf("KokkosP NVML Energy: Kernel energy CSV written to %s/%s (%" PRIu64 " kernels)\n", 
                   cwd, filename.c_str(), static_cast<uint64_t>(completed_kernels.size()));
        } else {
            printf("KokkosP NVML Energy: Kernel energy CSV written to %s (%" PRIu64 " kernels)\n", 
                   filename.c_str(), static_cast<uint64_t>(completed_kernels.size()));
        }
    }
}

void DataManager::write_region_data(const std::string& filename) const {
    if (completed_regions.empty()) return;

    FILE* regions_file = fopen(filename.c_str(), "w");
    if (regions_file) {
        fprintf(regions_file, "name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns,start_energy_mj,end_energy_mj,delta_energy_mj,average_power_w\n");
        for (const auto& region : completed_regions) {
            auto start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                region.start_time.time_since_epoch())
                                .count();
            auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            region.end_time.time_since_epoch())
                            .count();
            fprintf(regions_file, "%s,%s,%" PRId64 ",%" PRId64 ",%" PRId64 ",%llu,%llu,%llu,%.6f\n",
                    region.name.c_str(), 
                    region_type_to_string(region.type), 
                    start_ns,
                    end_ns, 
                    (int64_t)region.duration.count(),
                    region.start_energy_mj,
                    region.end_energy_mj,
                    region.delta_energy_mj,
                    region.average_power_w);
        }
        fclose(regions_file);
        char cwd[256];
        if (getcwd(cwd, 256) != nullptr) {
            printf("KokkosP NVML Energy: Region energy CSV written to %s/%s (%" PRIu64 " regions)\n", 
                   cwd, filename.c_str(), static_cast<uint64_t>(completed_regions.size()));
        } else {
            printf("KokkosP NVML Energy: Region energy CSV written to %s (%" PRIu64 " regions)\n", 
                   filename.c_str(), static_cast<uint64_t>(completed_regions.size()));
        }
    }
}

} // namespace NVMLEnergyProfiler
} // namespace KokkosTools
