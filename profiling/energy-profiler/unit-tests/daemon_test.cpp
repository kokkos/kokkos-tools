#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cassert>
#include <stdexcept>
#include "../common/daemon.hpp"

// Test counters and flags
static std::atomic<uint32_t> counter{0};
static std::atomic<uint32_t> fast_counter{0};
static std::atomic<uint32_t> slow_counter{0};
static std::atomic<bool> exception_thrown{false};

// Test functions
void hello_world() {
  std::cout << "Hello World (execution #" << counter.load() + 1 << ")"
            << std::endl;
  counter++;
}

void fast_function() {
  fast_counter++;
  // Very fast function (< 1ms)
}

void slow_function() {
  slow_counter++;
  // Simulate a function that takes longer than interval
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
}

void exception_function() {
  exception_thrown = true;
  throw std::runtime_error("Test exception in daemon function");
}

// Test utilities
bool test_basic_functionality() {
  std::cout << "\n=== Test 1: Basic Functionality ===" << std::endl;

  counter = 0;
  Daemon daemon(hello_world, 100);

  // Test initial state
  assert(!daemon.is_running());

  std::cout << "Starting daemon..." << std::endl;
  daemon.start();

  // Test running state
  assert(daemon.is_running());

  // Let it run for ~350ms (should execute ~3-4 times)
  std::this_thread::sleep_for(std::chrono::milliseconds(350));

  daemon.stop();

  // Test stopped state
  assert(!daemon.is_running());

  uint32_t final_count = counter.load();
  std::cout << "Daemon finished. Counter: " << final_count << std::endl;

  // Should have executed 3-4 times (allowing some tolerance for timing)
  bool success = (final_count >= 3 && final_count <= 4);
  std::cout << "Test 1 " << (success ? "PASSED" : "FAILED") << std::endl;
  return success;
}

bool test_timing_accuracy() {
  std::cout << "\n=== Test 2: Timing Accuracy ===" << std::endl;

  fast_counter = 0;
  Daemon daemon(fast_function, 50);  // 50ms interval

  auto start_time = std::chrono::high_resolution_clock::now();
  daemon.start();

  // Run for exactly 250ms
  std::this_thread::sleep_for(std::chrono::milliseconds(250));

  daemon.stop();
  auto end_time = std::chrono::high_resolution_clock::now();

  uint32_t executions  = fast_counter.load();
  auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  std::cout << "Executions: " << executions << std::endl;
  std::cout << "Actual duration: " << actual_duration.count() << "ms"
            << std::endl;

  // Should execute ~5 times (250ms / 50ms = 5)
  bool success = (executions >= 4 && executions <= 6);
  std::cout << "Test 2 " << (success ? "PASSED" : "FAILED") << std::endl;
  return success;
}

bool test_slow_function_handling() {
  std::cout << "\n=== Test 3: Slow Function Handling ===" << std::endl;

  slow_counter = 0;
  Daemon daemon(slow_function,
                100);  // 100ms interval, but function takes 150ms

  auto start_time = std::chrono::high_resolution_clock::now();
  daemon.start();

  // Run for 400ms
  std::this_thread::sleep_for(std::chrono::milliseconds(400));

  daemon.stop();
  auto end_time = std::chrono::high_resolution_clock::now();

  uint32_t executions  = slow_counter.load();
  auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  std::cout << "Executions: " << executions << std::endl;
  std::cout << "Actual duration: " << actual_duration.count() << "ms"
            << std::endl;

  // Should execute 2-3 times (each execution takes ~150ms, total time ~400ms)
  bool success = (executions >= 2 && executions <= 3);
  std::cout << "Test 3 " << (success ? "PASSED" : "FAILED") << std::endl;
  return success;
}

bool test_start_stop_edge_cases() {
  std::cout << "\n=== Test 4: Start/Stop Edge Cases ===" << std::endl;

  Daemon daemon(hello_world, 1000);
  bool success = true;

  // Test double start
  try {
    daemon.start();
    daemon.start();  // Should throw
    success = false;
    std::cout << "ERROR: Double start should have thrown exception"
              << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout << "Double start correctly threw: " << e.what() << std::endl;
  }

  daemon.stop();

  // Test double stop
  try {
    daemon.stop();  // Should throw
    success = false;
    std::cout << "ERROR: Double stop should have thrown exception" << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout << "Double stop correctly threw: " << e.what() << std::endl;
  }

  // Test stop without start
  Daemon daemon2(hello_world, 1000);
  try {
    daemon2.stop();  // Should throw
    success = false;
    std::cout << "ERROR: Stop without start should have thrown exception"
              << std::endl;
  } catch (const std::runtime_error& e) {
    std::cout << "Stop without start correctly threw: " << e.what()
              << std::endl;
  }

  std::cout << "Test 4 " << (success ? "PASSED" : "FAILED") << std::endl;
  return success;
}

bool test_thread_safety() {
  std::cout << "\n=== Test 5: Thread Safety ===" << std::endl;

  counter = 0;
  Daemon daemon(hello_world, 200);  // Fast interval

  daemon.start();

  // Check is_running from main thread while daemon is running
  bool running_check1 = daemon.is_running();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  bool running_check2 = daemon.is_running();

  daemon.stop();
  bool running_check3 = daemon.is_running();

  bool success = running_check1 && running_check2 && !running_check3;
  std::cout << "Running state checks: " << running_check1 << ", "
            << running_check2 << ", " << running_check3 << std::endl;
  std::cout << "Executions during test: " << counter.load() << std::endl;
  std::cout << "Test 5 " << (success ? "PASSED" : "FAILED") << std::endl;
  return success;
}

int main() {
  std::cout << "=== Daemon Comprehensive Test Suite ===" << std::endl;

  int passed = 0;
  int total  = 5;

  if (test_basic_functionality()) passed++;
  if (test_timing_accuracy()) passed++;
  if (test_slow_function_handling()) passed++;
  if (test_start_stop_edge_cases()) passed++;
  if (test_thread_safety()) passed++;

  std::cout << "\n=== Test Results ===" << std::endl;
  std::cout << "Passed: " << passed << "/" << total << std::endl;

  if (passed == total) {
    std::cout << "ALL TESTS PASSED! Daemon is working correctly." << std::endl;
    return 0;
  } else {
    std::cout << "Some tests failed. Please check the daemon implementation."
              << std::endl;
    return 1;
  }
}