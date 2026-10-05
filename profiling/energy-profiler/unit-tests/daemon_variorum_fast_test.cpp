#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <limits>
#include "../common/daemon.hpp"
#include "../provider/provider_variorum.hpp"

using namespace KokkosTools::EnergyProfiler;

// Global variables for the monitoring function
static VariorumProvider* g_variorum_provider = nullptr;
static std::atomic<uint32_t> g_sample_count{0};
static std::atomic<double> g_total_energy{0.0};
static std::atomic<double> g_min_power{std::numeric_limits<double>::max()};
static std::atomic<double> g_max_power{0.0};
static std::vector<double> g_power_samples;
static std::mutex g_samples_mutex;

void fast_power_monitoring_function() {
  if (!g_variorum_provider || !g_variorum_provider->is_initialized()) {
    return;
  }

  double current_power = 0.0;
  bool power_success =
      g_variorum_provider->get_total_power_usage(current_power);
  if (!power_success) {
    return;  // Skip this sample if we can't get power reading
  }

  // Update statistics atomically
  g_sample_count.fetch_add(1);

  // Accumulate energy (Power * Time)
  // Since we sample every 20ms, energy increment = power * 0.02 seconds
  double expected = g_total_energy.load();
  while (!g_total_energy.compare_exchange_weak(
      expected, expected + current_power * 0.02)) {
    // Loop until successful update
  }

  // Update min power
  double current_min = g_min_power.load();
  while (current_power < current_min &&
         !g_min_power.compare_exchange_weak(current_min, current_power)) {
    // Loop until successful update
  }

  // Update max power
  double current_max = g_max_power.load();
  while (current_power > current_max &&
         !g_max_power.compare_exchange_weak(current_max, current_power)) {
    // Loop until successful update
  }

  // Store sample for statistical analysis (thread-safe)
  {
    std::lock_guard<std::mutex> lock(g_samples_mutex);
    g_power_samples.push_back(current_power);
  }
}

double calculate_standard_deviation(const std::vector<double>& samples,
                                    double mean) {
  if (samples.size() <= 1) return 0.0;

  double sum_squared_diff = 0.0;
  for (double sample : samples) {
    double diff = sample - mean;
    sum_squared_diff += diff * diff;
  }

  return std::sqrt(sum_squared_diff / (samples.size() - 1));
}

bool test_daemon_variorum_fast_integration() {
  std::cout << "=== Fast Daemon + Variorum Integration Test (20ms sampling) ==="
            << std::endl;

  // Reset global counters
  g_sample_count = 0;
  g_total_energy = 0.0;
  g_min_power    = std::numeric_limits<double>::max();
  g_max_power    = 0.0;
  g_power_samples.clear();

  // Initialize Variorum provider
  std::cout << "\n1. Initializing Variorum provider..." << std::endl;
  VariorumProvider variorum_provider;
  if (!variorum_provider.initialize()) {
    std::cout << "ERROR: Failed to initialize Variorum provider" << std::endl;
    return false;
  }

  g_variorum_provider = &variorum_provider;
  std::cout << "SUCCESS: Variorum provider initialized with "
            << variorum_provider.get_device_count() << " device(s)"
            << std::endl;

  // Create daemon with 20ms interval
  std::cout << "\n2. Creating daemon with 20ms monitoring interval..."
            << std::endl;
  Daemon power_daemon(fast_power_monitoring_function, 20);

  // Start monitoring
  std::cout << "\n3. Starting fast power monitoring..." << std::endl;
  power_daemon.start();
  std::cout << "SUCCESS: Fast power monitoring started" << std::endl;

  // Let it run for 2 seconds
  std::cout << "\n4. Monitoring for 2 seconds (high frequency sampling)..."
            << std::endl;
  std::cout << "   (No real-time output to avoid saturation)" << std::endl;

  auto start_time = std::chrono::high_resolution_clock::now();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto end_time = std::chrono::high_resolution_clock::now();

  // Stop monitoring
  std::cout << "\n5. Stopping power monitoring..." << std::endl;
  power_daemon.stop();
  std::cout << "SUCCESS: Power monitoring stopped" << std::endl;

  // Calculate actual monitoring duration
  auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  // Analyze collected data
  std::cout << "\n=== Statistical Analysis ===" << std::endl;

  uint32_t total_samples = g_sample_count.load();
  double total_energy    = g_total_energy.load();
  double min_power       = g_min_power.load();
  double max_power       = g_max_power.load();

  std::cout << "Monitoring duration: " << actual_duration.count() << " ms"
            << std::endl;
  std::cout << "Total samples collected: " << total_samples << std::endl;
  std::cout << "Expected samples (50 Hz): " << (actual_duration.count() / 20)
            << std::endl;
  std::cout << "Sampling efficiency: " << std::fixed << std::setprecision(1)
            << (100.0 * total_samples / (actual_duration.count() / 20.0)) << "%"
            << std::endl;

  if (total_samples > 0) {
    double avg_power = total_energy / (total_samples * 0.02);

    std::cout << "\n=== Power Statistics ===" << std::endl;
    std::cout << "Average power: " << std::fixed << std::setprecision(2)
              << avg_power << " W" << std::endl;
    std::cout << "Minimum power: " << std::fixed << std::setprecision(2)
              << min_power << " W" << std::endl;
    std::cout << "Maximum power: " << std::fixed << std::setprecision(2)
              << max_power << " W" << std::endl;
    std::cout << "Power range: " << std::fixed << std::setprecision(2)
              << (max_power - min_power) << " W" << std::endl;
    std::cout << "Total energy consumed: " << std::fixed << std::setprecision(3)
              << total_energy << " J" << std::endl;

    // Calculate additional statistics from stored samples
    {
      std::lock_guard<std::mutex> lock(g_samples_mutex);
      if (!g_power_samples.empty()) {
        std::sort(g_power_samples.begin(), g_power_samples.end());

        size_t n = g_power_samples.size();
        double median =
            (n % 2 == 0)
                ? (g_power_samples[n / 2 - 1] + g_power_samples[n / 2]) / 2.0
                : g_power_samples[n / 2];

        double q1 = g_power_samples[n / 4];
        double q3 = g_power_samples[3 * n / 4];

        double std_dev =
            calculate_standard_deviation(g_power_samples, avg_power);

        std::cout << "\n=== Extended Statistics ===" << std::endl;
        std::cout << "Median power: " << std::fixed << std::setprecision(2)
                  << median << " W" << std::endl;
        std::cout << "Q1 (25th percentile): " << std::fixed
                  << std::setprecision(2) << q1 << " W" << std::endl;
        std::cout << "Q3 (75th percentile): " << std::fixed
                  << std::setprecision(2) << q3 << " W" << std::endl;
        std::cout << "Standard deviation: " << std::fixed
                  << std::setprecision(2) << std_dev << " W" << std::endl;
        std::cout << "Coefficient of variation: " << std::fixed
                  << std::setprecision(1) << (100.0 * std_dev / avg_power)
                  << "%" << std::endl;
      }
    }

    // Show per-device breakdown if multiple devices
    size_t device_count = variorum_provider.get_device_count();
    if (device_count > 1) {
      std::cout << "\n=== Per-Device Final Readings ===" << std::endl;
      for (size_t i = 0; i < device_count; ++i) {
        double device_power = 0.0;
        bool device_success =
            variorum_provider.get_device_power_usage(i, device_power);
        std::string device_name = variorum_provider.get_device_name(i);

        if (device_success) {
          std::cout << "  " << device_name << ": " << std::fixed
                    << std::setprecision(2) << device_power << " W"
                    << std::endl;
        } else {
          std::cout << "  " << device_name << ": Failed to read power"
                    << std::endl;
        }
      }
    }
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
    if (test_daemon_variorum_fast_integration()) {
      std::cout << "\nFast integration test PASSED!" << std::endl;
      return 0;
    } else {
      std::cout << "\nFast integration test FAILED!" << std::endl;
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "\nTest failed with exception: " << e.what() << std::endl;
    return 1;
  }
}