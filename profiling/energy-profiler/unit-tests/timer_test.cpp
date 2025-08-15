#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include "../common/timer_system.hpp"

using namespace KokkosTools::Timer;

// Test helper function to check if a value is within expected range
bool is_within_range(uint64_t actual, uint64_t expected, uint64_t tolerance) {
  return (actual >= expected - tolerance) && (actual <= expected + tolerance);
}

bool test_basic_timing() {
  std::cout << "=== Test Basic Timing ===" << std::endl;

  KernelTimerTool timer;

  // Test single timing
  timer.begin_parallel_for("test_kernel", 0, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_parallel_for(1);

  const auto& timings = timer.get_kernel_timings();
  if (timings.size() != 1) {
    std::cout << "ERROR: Expected 1 timing, got " << timings.size()
              << std::endl;
    return false;
  }

  const auto& timing = timings[0];
  uint64_t duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timing.duration)
          .count();

  if (!is_within_range(duration_ms, 2, 2)) {  // 2ms ± 2ms tolerance
    std::cout << "ERROR: Duration should be ~2ms, got " << duration_ms << "ms"
              << std::endl;
    return false;
  }

  if (timing.name != "test_kernel") {
    std::cout << "ERROR: Wrong name, expected 'test_kernel', got '"
              << timing.name << "'" << std::endl;
    return false;
  }

  if (timing.type != RegionType::ParallelFor) {
    std::cout << "ERROR: Wrong region type" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Basic timing works correctly (duration: "
            << duration_ms << "ms)" << std::endl;
  return true;
}

bool test_multiple_timings() {
  std::cout << "\n=== Test Multiple Timings ===" << std::endl;

  KernelTimerTool timer;

  // Start multiple timings with different types
  timer.begin_parallel_for("kernel_1", 0, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  timer.end_parallel_for(1);

  uint64_t kID2 = 2;
  timer.begin_parallel_reduce("kernel_2", 0, &kID2);
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.end_parallel_reduce(2);

  timer.push_profile_region("region_1");
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.pop_profile_region();

  const auto& kernel_timings = timer.get_kernel_timings();
  const auto& region_timings = timer.get_region_timings();

  if (kernel_timings.size() != 2) {
    std::cout << "ERROR: Expected 2 kernel timings, got "
              << kernel_timings.size() << std::endl;
    return false;
  }

  if (region_timings.size() != 1) {
    std::cout << "ERROR: Expected 1 region timing, got "
              << region_timings.size() << std::endl;
    return false;
  }

  // Check individual durations
  uint64_t duration1_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              kernel_timings[0].duration)
                              .count();
  uint64_t duration2_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              kernel_timings[1].duration)
                              .count();
  uint64_t duration3_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              region_timings[0].duration)
                              .count();

  if (!is_within_range(duration1_ms, 1, 1)) {
    std::cout << "ERROR: Duration1 should be ~1ms, got " << duration1_ms << "ms"
              << std::endl;
    return false;
  }

  if (!is_within_range(duration2_ms, 2, 2)) {
    std::cout << "ERROR: Duration2 should be ~2ms, got " << duration2_ms << "ms"
              << std::endl;
    return false;
  }

  if (!is_within_range(duration3_ms, 2, 2)) {
    std::cout << "ERROR: Duration3 should be ~2ms, got " << duration3_ms << "ms"
              << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Multiple timings work correctly" << std::endl;
  std::cout << "  Duration1: " << duration1_ms << "ms" << std::endl;
  std::cout << "  Duration2: " << duration2_ms << "ms" << std::endl;
  std::cout << "  Duration3: " << duration3_ms << "ms" << std::endl;
  return true;
}

bool test_region_types() {
  std::cout << "\n=== Test Region Types ===" << std::endl;

  KernelTimerTool timer;

  // Test all region types
  timer.begin_parallel_for("parallel_for", 0, 1);
  timer.end_parallel_for(1);

  uint64_t kID2 = 2;
  timer.begin_parallel_scan("parallel_scan", 0, &kID2);
  timer.end_parallel_scan(2);

  uint64_t kID3 = 3;
  timer.begin_parallel_reduce("parallel_reduce", 0, &kID3);
  timer.end_parallel_reduce(3);

  // Simulate deep copy
  char dst_name[] = "deep_copy";
  char dst_ptr    = 'x';
  char src_name[] = "source";
  char src_ptr    = 'y';
  timer.begin_deep_copy(Kokkos::Tools::SpaceHandle{}, dst_name, &dst_ptr,
                        Kokkos::Tools::SpaceHandle{}, src_name, &src_ptr, 64);
  timer.end_deep_copy();

  timer.push_profile_region("user_region");
  timer.pop_profile_region();

  const auto& kernel_timings   = timer.get_kernel_timings();
  const auto& region_timings   = timer.get_region_timings();
  const auto& deepcopy_timings = timer.get_deep_copy_timings();

  if (kernel_timings.size() != 3) {
    std::cout << "ERROR: Expected 3 kernel timings, got "
              << kernel_timings.size() << std::endl;
    return false;
  }

  if (region_timings.size() != 1) {
    std::cout << "ERROR: Expected 1 region timing, got "
              << region_timings.size() << std::endl;
    return false;
  }

  if (deepcopy_timings.size() != 1) {
    std::cout << "ERROR: Expected 1 deep copy timing, got "
              << deepcopy_timings.size() << std::endl;
    return false;
  }

  // Verify region types
  if (kernel_timings[0].type != RegionType::ParallelFor ||
      kernel_timings[1].type != RegionType::ParallelScan ||
      kernel_timings[2].type != RegionType::ParallelReduce ||
      deepcopy_timings[0].type != RegionType::DeepCopy ||
      region_timings[0].type != RegionType::UserRegion) {
    std::cout << "ERROR: Region types not correctly set" << std::endl;
    return false;
  }

  // Verify names
  if (kernel_timings[0].name != "parallel_for" ||
      kernel_timings[1].name != "parallel_scan" ||
      kernel_timings[2].name != "parallel_reduce" ||
      deepcopy_timings[0].name != "deep_copy" ||
      region_timings[0].name != "user_region") {
    std::cout << "ERROR: Names not correctly set" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: All region types work correctly" << std::endl;
  return true;
}

bool test_error_handling() {
  std::cout << "\n=== Test Error Handling ===" << std::endl;

  KernelTimerTool timer;

  // Test multiple calls to end without begin (should not crash)
  timer.end_parallel_for(999);  // This should not crash
  timer.pop_profile_region();   // This should not crash

  // Test normal operation after error calls
  timer.begin_parallel_for("test", 0, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  timer.end_parallel_for(1);

  const auto& timings = timer.get_kernel_timings();
  if (timings.size() != 1) {
    std::cout << "ERROR: Should have exactly 1 timing" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Error handling works correctly" << std::endl;
  return true;
}

bool test_precision() {
  std::cout << "\n=== Test Precision ===" << std::endl;

  KernelTimerTool timer;

  // Test very short timing (should be 0 or 1 ms)
  timer.begin_parallel_for("short_op", 0, 1);
  // No sleep - immediate end
  timer.end_parallel_for(1);

  // Test longer timing for better precision
  timer.begin_parallel_for("long_op", 0, 2);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  timer.end_parallel_for(2);

  const auto& timings = timer.get_kernel_timings();
  if (timings.size() != 2) {
    std::cout << "ERROR: Expected 2 timings, got " << timings.size()
              << std::endl;
    return false;
  }

  uint64_t short_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timings[0].duration)
          .count();
  uint64_t long_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timings[1].duration)
          .count();

  if (short_duration_ms > 2) {  // Should be very small
    std::cout << "WARNING: Short duration is " << short_duration_ms
              << "ms (expected ≤2ms)" << std::endl;
  }

  if (!is_within_range(long_duration_ms, 10, 5)) {
    std::cout << "ERROR: Long duration should be ~10ms, got "
              << long_duration_ms << "ms" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Precision test passed" << std::endl;
  std::cout << "  Short duration: " << short_duration_ms << "ms" << std::endl;
  std::cout << "  Long duration: " << long_duration_ms << "ms" << std::endl;
  return true;
}

bool test_concurrent_timings() {
  std::cout << "\n=== Test Concurrent Timings ===" << std::endl;

  KernelTimerTool timer;

  // Start overlapping timings using regions (since kernels can't overlap in
  // this implementation)
  timer.push_profile_region("outer");
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  timer.push_profile_region("inner");
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  timer.pop_profile_region();  // End inner first

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  timer.pop_profile_region();  // End outer last

  const auto& region_timings = timer.get_region_timings();
  if (region_timings.size() != 2) {
    std::cout << "ERROR: Expected 2 region timings, got "
              << region_timings.size() << std::endl;
    return false;
  }

  uint64_t inner_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          region_timings[0].duration)
          .count();
  uint64_t outer_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          region_timings[1].duration)
          .count();

  // Outer should be longer than inner
  if (outer_duration_ms <= inner_duration_ms) {
    std::cout << "ERROR: Outer duration (" << outer_duration_ms
              << "ms) should be greater than inner duration ("
              << inner_duration_ms << "ms)" << std::endl;
    return false;
  }

  // Check approximate durations
  if (!is_within_range(inner_duration_ms, 2, 2)) {
    std::cout << "ERROR: Inner duration should be ~2ms, got "
              << inner_duration_ms << "ms" << std::endl;
    return false;
  }

  if (!is_within_range(outer_duration_ms, 4, 2)) {  // 1 + 2 + 1 = 4ms
    std::cout << "ERROR: Outer duration should be ~4ms, got "
              << outer_duration_ms << "ms" << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Concurrent timings work correctly" << std::endl;
  std::cout << "  Outer duration: " << outer_duration_ms << "ms" << std::endl;
  std::cout << "  Inner duration: " << inner_duration_ms << "ms" << std::endl;
  return true;
}

bool very_long_timing() {
  std::cout << "\n=== Test Very Long Timing ===" << std::endl;

  KernelTimerTool timer;

  timer.begin_parallel_for("very_long_op", 0, 1);
  std::this_thread::sleep_for(
      std::chrono::milliseconds(50));  // Sleep for 50ms instead of 1 second
  timer.end_parallel_for(1);

  const auto& timings = timer.get_kernel_timings();
  if (timings.size() != 1) {
    std::cout << "ERROR: Expected 1 timing, got " << timings.size()
              << std::endl;
    return false;
  }

  uint64_t duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(timings[0].duration)
          .count();

  if (!is_within_range(duration_ms, 50, 10)) {  // Allow some margin of error
    std::cout << "ERROR: Duration should be ~50ms, got " << duration_ms << "ms"
              << std::endl;
    return false;
  }

  std::cout << "SUCCESS: Very long timing works correctly (duration: "
            << duration_ms << "ms)" << std::endl;
  return true;
}

int main() {
  std::cout << "Running KernelTimerTool Tests..." << std::endl;
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