#include <iostream>
#include <chrono>
#include <thread>
#include "../provider/provider_nvml.hpp"

void test_nvml_provider() {
  std::cout << "=== NVML Provider Test ===" << std::endl;

  NVMLProvider provider;

  // Test initialization
  std::cout << "\n1. Testing initialization..." << std::endl;
  if (!provider.initialize()) {
    std::cout << "ERROR: Failed to initialize NVML provider" << std::endl;
    return;
  }
  std::cout << "SUCCESS: NVML provider initialized successfully" << std::endl;

  // Test device discovery
  std::cout << "\n2. Testing device discovery..." << std::endl;
  size_t device_count = provider.get_device_count();
  std::cout << "Found " << device_count << " device(s)" << std::endl;

  if (device_count == 0) {
    std::cout << "ERROR: No devices found" << std::endl;
    return;
  }

  // Display device information
  std::cout << "\n3. Device information:" << std::endl;
  for (size_t i = 0; i < device_count; ++i) {
    std::string name = provider.get_device_name(i);
    std::cout << "  Device " << i << ": " << name << std::endl;
  }

  // Test power readings
  std::cout << "\n4. Testing power readings..." << std::endl;
  for (int sample = 0; sample < 5; ++sample) {
    std::cout << "Sample " << (sample + 1) << ":" << std::endl;

    // Individual device power
    for (size_t i = 0; i < device_count; ++i) {
      double power = provider.get_device_power_usage(i);
      if (power >= 0.0) {
        std::cout << "  Device " << i << ": " << power << " W" << std::endl;
      } else {
        std::cout << "  Device " << i << ": Failed to read power" << std::endl;
      }
    }

    // Individual device direct power
    for (size_t i = 0; i < device_count; ++i) {
      double direct_power = provider.get_device_power_usage_direct(i);
      if (direct_power >= 0.0) {
        std::cout << "  Device " << i << " (Direct): " << direct_power << " W"
                  << std::endl;
      } else {
        std::cout << "  Device " << i
                  << " (Direct): Failed to read direct power" << std::endl;
      }
    }

    // Current energy consumption
    for (size_t i = 0; i < device_count; ++i) {
      double energy = provider.get_current_energy_consumption(i);
      if (energy >= 0.0) {
        std::cout << "  Device " << i << " Energy: " << energy << " J"
                  << std::endl;
      } else {
        std::cout << "  Device " << i << " Energy: Failed to read energy"
                  << std::endl;
      }
    }

    // Total power
    double total_power = provider.get_total_power_usage();
    std::cout << "  Total Power: " << total_power << " W" << std::endl;

    if (sample < 4) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Test finalization
  std::cout << "\n5. Testing finalization..." << std::endl;
  provider.finalize();
  std::cout << "SUCCESS: NVML provider finalized successfully" << std::endl;

  std::cout << "\n=== Test Completed ===" << std::endl;
}

int main() {
  try {
    test_nvml_provider();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}