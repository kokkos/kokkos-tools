#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
#include "../common/daemon.hpp"
#include "../provider/provider_variorum.hpp"

using namespace KokkosTools::EnergyProfiler;

// Global variables for the monitoring function
static VariorumProvider* g_variorum_provider = nullptr;
static std::atomic<uint32_t> g_sample_count{0};
static std::atomic<double> g_total_energy{0.0};
static std::atomic<double> g_last_power{0.0};

void power_monitoring_function() {
  if (!g_variorum_provider || !g_variorum_provider->is_initialized()) {
    std::cout << "ERROR: Variorum provider not initialized" << std::endl;
    return;
  }

  double current_power = 0.0;
  Result power_result =
      g_variorum_provider->get_total_power_usage(current_power);
  if (!power_result.is_success()) {
    std::cout << "ERROR: Failed to get power reading: " << power_result.message
              << std::endl;
    return;
  }

  g_last_power.store(current_power);

  // Accumulate energy (Power * Time)
  // Since we sample every 1000ms, energy increment = power * 1.0 seconds
  double expected = g_total_energy.load();
  while (!g_total_energy.compare_exchange_weak(
      expected, expected + current_power * 1.0)) {
    // Loop until successful update
  }

  uint32_t sample_num = g_sample_count.fetch_add(1) + 1;

  std::cout << std::fixed << std::setprecision(2) << "Sample " << sample_num
            << ": " << current_power
            << " W (Total Energy: " << g_total_energy.load() << " J)"
            << std::endl;

  // Show individual device power if multiple devices
  size_t device_count = g_variorum_provider->get_device_count();
  if (device_count > 1) {
    for (size_t i = 0; i < device_count; ++i) {
      double device_power = 0.0;
      Result device_result =
          g_variorum_provider->get_device_power_usage(i, device_power);
      if (device_result.is_success()) {
        std::cout << "  " << g_variorum_provider->get_device_name(i) << ": "
                  << device_power << " W" << std::endl;
      }
    }
  }
}

bool test_daemon_variorum_integration() {
  std::cout << "=== Daemon + Variorum Integration Test ===" << std::endl;

  // Reset global counters
  g_sample_count = 0;
  g_total_energy = 0.0;
  g_last_power   = 0.0;

  // Initialize Variorum provider
  std::cout << "\n1. Initializing Variorum provider..." << std::endl;
  VariorumProvider variorum_provider;
  Result init_result = variorum_provider.initialize();
  if (!init_result.is_success()) {
    std::cout << "ERROR: Failed to initialize Variorum provider: "
              << init_result.message << std::endl;
    return false;
  }

  g_variorum_provider = &variorum_provider;
  std::cout << "SUCCESS: Variorum provider initialized with "
            << variorum_provider.get_device_count() << " device(s)"
            << std::endl;

  // Create daemon with 1-second interval
  std::cout << "\n2. Creating daemon with 1-second monitoring interval..."
            << std::endl;
  Daemon power_daemon(power_monitoring_function, 1000);

  // Start monitoring
  std::cout << "\n3. Starting power monitoring..." << std::endl;
  power_daemon.start();
  std::cout << "SUCCESS: Power monitoring started" << std::endl;

  // Let it run for 2 seconds
  std::cout << "\n4. Monitoring for 2 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));

  // Stop monitoring
  std::cout << "\n5. Stopping power monitoring..." << std::endl;
  power_daemon.stop();
  std::cout << "SUCCESS: Power monitoring stopped" << std::endl;

  // Display final statistics
  std::cout << "\n=== Final Statistics ===" << std::endl;
  std::cout << "Total samples: " << g_sample_count.load() << std::endl;
  std::cout << "Last power reading: " << std::fixed << std::setprecision(2)
            << g_last_power.load() << " W" << std::endl;
  std::cout << "Total energy consumed: " << std::fixed << std::setprecision(2)
            << g_total_energy.load() << " J" << std::endl;

  if (g_sample_count.load() > 0) {
    double avg_power = g_total_energy.load() / (g_sample_count.load() * 1.0);
    std::cout << "Average power: " << std::fixed << std::setprecision(2)
              << avg_power << " W" << std::endl;
  }

  // Cleanup
  std::cout << "\n6. Cleaning up..." << std::endl;
  g_variorum_provider = nullptr;
  variorum_provider.finalize();
  std::cout << "SUCCESS: Cleanup completed" << std::endl;

  return true;
}

int main() {
  try {
    if (test_daemon_variorum_integration()) {
      std::cout << "\nIntegration test PASSED!" << std::endl;
      return 0;
    } else {
      std::cout << "\nIntegration test FAILED!" << std::endl;
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
    return 1;
  }
}