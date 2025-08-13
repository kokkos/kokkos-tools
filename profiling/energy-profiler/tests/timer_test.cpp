#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include "../common/timer.hpp"

// Test helper function to check if a value is within expected range
bool is_within_range(uint64_t actual, uint64_t expected, uint64_t tolerance) {
  return (actual >= expected - tolerance) && (actual <= expected + tolerance);
}

bool test_basic_timing() {
  std::cout << "=== Test Basic Timing ===" << std::endl;

  EnergyTimer timer;

  // Test single timing
  timer.start_timing(1, RegionType::ParallelFor, "test_kernel");
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_timing(1);

  auto& timings = timer.get_timings();
  if (timings.size() != 1) {
    std::cout << "ERROR: Expected 1 timing, got " << timings.size()
              << std::endl;
    return false;
  }

  auto& timing = timings[1];
  if (!timing.is_ended()) {
    std::cout << "ERROR: Timing should be ended" << std::endl;
    return false;
  }

  uint64_t duration = timing.get_duration_ms();
  if (!is_within_range(duration, 2, 2)) {  // 2ms ± 2ms tolerance
    std::cout << "ERROR: Duration should be ~2ms, got " << duration << "ms"
              << std::endl;
    return false;
  }

  if (timing.name_ != "test_kernel") {
    std::cout << "ERROR: Wrong name, expected 'test_kernel', got '"
              << timing.name_ << "'" << std::endl;
    return false;
  }

  if (timing.region_type_ != RegionType::ParallelFor) {
    std::cout << "ERROR: Wrong region type" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Basic timing works correctly (duration: " << duration
            << "ms)" << std::endl;
  return true;
}

bool test_multiple_timings() {
  std::cout << "\n=== Test Multiple Timings ===" << std::endl;

  EnergyTimer timer;

  // Start multiple timings
  timer.start_timing(1, RegionType::ParallelFor, "kernel_1");
  timer.start_timing(2, RegionType::ParallelReduce, "kernel_2");
  timer.start_timing(3, RegionType::UserRegion, "region_1");

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  timer.end_timing(1);

  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_timing(2);

  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_timing(3);

  auto& timings = timer.get_timings();
  if (timings.size() != 3) {
    std::cout << "ERROR: Expected 3 timings, got " << timings.size()
              << std::endl;
    return false;
  }

  // Check individual durations
  uint64_t duration1 = timings[1].get_duration_ms();
  uint64_t duration2 = timings[2].get_duration_ms();
  uint64_t duration3 = timings[3].get_duration_ms();

  if (!is_within_range(duration1, 1, 1)) {
    std::cout << "ERROR: Duration1 should be ~1ms, got " << duration1 << "ms"
              << std::endl;
    return false;
  }

  if (!is_within_range(duration2, 3, 2)) {  // 1 + 2 = 3ms
    std::cout << "ERROR: Duration2 should be ~3ms, got " << duration2 << "ms"
              << std::endl;
    return false;
  }

  if (!is_within_range(duration3, 5, 2)) {  // 1 + 2 + 2 = 5ms
    std::cout << "ERROR: Duration3 should be ~5ms, got " << duration3 << "ms"
              << std::endl;
    return false;
  }

  // Check that duration2 > duration1 and duration3 > duration2
  if (duration2 <= duration1) {
    std::cout << "ERROR: Duration2 should be greater than duration1"
              << std::endl;
    return false;
  }

  if (duration3 <= duration2) {
    std::cout << "ERROR: Duration3 should be greater than duration2"
              << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Multiple timings work correctly" << std::endl;
  std::cout << "  Duration1: " << duration1 << "ms" << std::endl;
  std::cout << "  Duration2: " << duration2 << "ms" << std::endl;
  std::cout << "  Duration3: " << duration3 << "ms" << std::endl;
  return true;
}

bool test_region_types() {
  std::cout << "\n=== Test Region Types ===" << std::endl;

  EnergyTimer timer;

  // Test all region types
  timer.start_timing(1, RegionType::ParallelFor, "parallel_for");
  timer.start_timing(2, RegionType::ParallelScan, "parallel_scan");
  timer.start_timing(3, RegionType::ParallelReduce, "parallel_reduce");
  timer.start_timing(4, RegionType::DeepCopy, "deep_copy");
  timer.start_timing(5, RegionType::UserRegion, "user_region");
  timer.start_timing(6, RegionType::Unknown, "unknown_op");

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  timer.end_timing(1);
  timer.end_timing(2);
  timer.end_timing(3);
  timer.end_timing(4);
  timer.end_timing(5);
  timer.end_timing(6);

  auto& timings = timer.get_timings();
  if (timings.size() != 6) {
    std::cout << "ERROR: Expected 6 timings, got " << timings.size()
              << std::endl;
    return false;
  }

  // Verify region types
  if (timings[1].region_type_ != RegionType::ParallelFor ||
      timings[2].region_type_ != RegionType::ParallelScan ||
      timings[3].region_type_ != RegionType::ParallelReduce ||
      timings[4].region_type_ != RegionType::DeepCopy ||
      timings[5].region_type_ != RegionType::UserRegion ||
      timings[6].region_type_ != RegionType::Unknown) {
    std::cout << "ERROR: Region types not correctly set" << std::endl;
    return false;
  }

  // Verify names
  if (timings[1].name_ != "parallel_for" ||
      timings[2].name_ != "parallel_scan" ||
      timings[3].name_ != "parallel_reduce" ||
      timings[4].name_ != "deep_copy" || timings[5].name_ != "user_region" ||
      timings[6].name_ != "unknown_op") {
    std::cout << "ERROR: Names not correctly set" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: All region types work correctly" << std::endl;
  return true;
}

bool test_error_handling() {
  std::cout << "\n=== Test Error Handling ===" << std::endl;

  EnergyTimer timer;

  // Test ending non-existent timing (should not crash)
  timer.end_timing(999);  // This should not crash

  // Test getting duration before ending
  timer.start_timing(1, RegionType::ParallelFor, "test");
  auto& timings = timer.get_timings();

  if (timings[1].is_ended()) {
    std::cout << "ERROR: Timing should not be ended yet" << std::endl;
    return false;
  }

  // End the timing
  timer.end_timing(1);

  if (!timings[1].is_ended()) {
    std::cout << "ERROR: Timing should be ended now" << std::endl;
    return false;
  }

  // Test ending the same timing twice (should not crash)
  timer.end_timing(1);

  std::cout << "SUCCESS: Error handling works correctly" << std::endl;
  return true;
}

bool test_precision() {
  std::cout << "\n=== Test Precision ===" << std::endl;

  EnergyTimer timer;

  // Test very short timing (should be 0 or 1 ms)
  timer.start_timing(1, RegionType::ParallelFor, "short_op");
  // No sleep - immediate end
  timer.end_timing(1);

  auto& timings           = timer.get_timings();
  uint64_t short_duration = timings[1].get_duration_ms();

  if (short_duration > 2) {  // Should be very small
    std::cout << "WARNING: Short duration is " << short_duration
              << "ms (expected ≤2ms)" << std::endl;
  }

  // Test longer timing for better precision
  timer.start_timing(2, RegionType::ParallelFor, "long_op");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  timer.end_timing(2);

  uint64_t long_duration = timings[2].get_duration_ms();

  if (!is_within_range(long_duration, 10, 5)) {
    std::cout << "ERROR: Long duration should be ~10ms, got " << long_duration
              << "ms" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Precision test passed" << std::endl;
  std::cout << "  Short duration: " << short_duration << "ms" << std::endl;
  std::cout << "  Long duration: " << long_duration << "ms" << std::endl;
  return true;
}

bool test_concurrent_timings() {
  std::cout << "\n=== Test Concurrent Timings ===" << std::endl;

  EnergyTimer timer;

  // Start overlapping timings
  timer.start_timing(1, RegionType::ParallelFor, "outer");
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  timer.start_timing(2, RegionType::ParallelReduce, "inner");
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_timing(2);  // End inner first

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  timer.end_timing(1);  // End outer last

  auto& timings           = timer.get_timings();
  uint64_t outer_duration = timings[1].get_duration_ms();
  uint64_t inner_duration = timings[2].get_duration_ms();

  // Outer should be longer than inner
  if (outer_duration <= inner_duration) {
    std::cout << "ERROR: Outer duration (" << outer_duration
              << "ms) should be greater than inner duration (" << inner_duration
              << "ms)" << std::endl;
    return false;
  }

  // Check approximate durations
  if (!is_within_range(inner_duration, 2, 2)) {
    std::cout << "ERROR: Inner duration should be ~2ms, got " << inner_duration
              << "ms" << std::endl;
    return false;
  }

  if (!is_within_range(outer_duration, 4, 2)) {  // 1 + 2 + 1 = 4ms
    std::cout << "ERROR: Outer duration should be ~4ms, got " << outer_duration
              << "ms" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Concurrent timings work correctly" << std::endl;
  std::cout << "  Outer duration: " << outer_duration << "ms" << std::endl;
  std::cout << "  Inner duration: " << inner_duration << "ms" << std::endl;
  return true;
}

bool very_long_timing() {
  std::cout << "\n=== Test Very Long Timing ===" << std::endl;

  EnergyTimer timer;

  timer.start_timing(1, RegionType::ParallelFor, "very_long_op");
  std::this_thread::sleep_for(
      std::chrono::milliseconds(50));  // Sleep for 50ms instead of 1 second
  timer.end_timing(1);

  auto& timings     = timer.get_timings();
  uint64_t duration = timings[1].get_duration_ms();

  if (!is_within_range(duration, 50, 10)) {  // Allow some margin of error
    std::cout << "ERROR: Duration should be ~50ms, got " << duration << "ms"
              << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Very long timing works correctly (duration: "
            << duration << "ms)" << std::endl;
  return true;
}

int main() {
  std::cout << "Running EnergyTimer Tests..." << std::endl;
  std::cout << "=============================" << std::endl;

  bool all_passed = true;

  all_passed &= test_basic_timing();
  all_passed &= test_multiple_timings();
  all_passed &= test_region_types();
  all_passed &= test_error_handling();
  all_passed &= test_precision();
  all_passed &= test_concurrent_timings();
  all_passed &= very_long_timing();

  std::cout << "\n=============================" << std::endl;
  if (all_passed) {
    std::cout << "ALL TESTS PASSED!" << std::endl;
    return 0;
  } else {
    std::cout << "SOME TESTS FAILED!" << std::endl;
    return 1;
  }
}