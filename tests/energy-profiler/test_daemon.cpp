// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <atomic>
#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "daemon.hpp"

using KokkosTools::EnergyProfiler::Daemon;
using namespace std::chrono_literals;

//! Sampling interval of the energy profiler (50 Hz).
constexpr auto sampling_interval = 20ms;

// Timing bounds are loose on purpose: shared CI runners can delay a thread by
// tens of milliseconds.

TEST(EnergyProfilerDaemonTest, calls_periodically) {
  std::atomic<int> calls{0};
  Daemon daemon([&] { ++calls; }, sampling_interval);
  EXPECT_FALSE(daemon.is_running());

  daemon.start();
  EXPECT_TRUE(daemon.is_running());
  std::this_thread::sleep_for(300ms);
  daemon.stop();
  EXPECT_FALSE(daemon.is_running());

  // About 15 calls in 300 ms.
  EXPECT_GE(calls.load(), 5);
  EXPECT_LE(calls.load(), 20);
}

TEST(EnergyProfilerDaemonTest, repeated_start_and_stop) {
  Daemon daemon([] {}, sampling_interval);
  daemon.start();
  daemon.start();
  EXPECT_TRUE(daemon.is_running());

  daemon.stop();
  daemon.stop();
  EXPECT_FALSE(daemon.is_running());
}

TEST(EnergyProfilerDaemonTest, restart_after_stop) {
  std::atomic<int> calls{0};
  Daemon daemon([&] { ++calls; }, sampling_interval);

  daemon.start();
  std::this_thread::sleep_for(100ms);
  daemon.stop();
  const int first_run = calls.load();

  daemon.start();
  std::this_thread::sleep_for(100ms);
  daemon.stop();

  EXPECT_GT(first_run, 0);
  EXPECT_GT(calls.load(), first_run);
}

TEST(EnergyProfilerDaemonTest, stop_does_not_wait) {
  Daemon daemon([] {}, 10s);
  daemon.start();
  std::this_thread::sleep_for(20ms);

  const auto begin = std::chrono::steady_clock::now();
  daemon.stop();
  EXPECT_LT(std::chrono::steady_clock::now() - begin, 1s);
}

TEST(EnergyProfilerDaemonTest, destructor_stops_daemon) {
  std::atomic<int> calls{0};
  {
    Daemon daemon([&] { ++calls; }, sampling_interval);
    daemon.start();
    std::this_thread::sleep_for(50ms);
  }
  const int after_destruction = calls.load();
  std::this_thread::sleep_for(50ms);

  EXPECT_GT(after_destruction, 0);
  EXPECT_EQ(calls.load(), after_destruction);
}

TEST(EnergyProfilerDaemonTest, slow_function) {
  std::atomic<int> calls{0};
  Daemon daemon(
      [&] {
        ++calls;
        std::this_thread::sleep_for(50ms);
      },
      sampling_interval);

  daemon.start();
  std::this_thread::sleep_for(300ms);
  daemon.stop();

  // One call every 50 ms instead of every 20 ms: about 6 in 300 ms.
  EXPECT_GE(calls.load(), 2);
  EXPECT_LE(calls.load(), 8);
}
