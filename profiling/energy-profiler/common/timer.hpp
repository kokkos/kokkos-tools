#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <deque>

enum class RegionType {
  Unknown,
  ParallelFor,
  ParallelReduce,
  ParallelScan,
  DeepCopy,
  UserRegion
};

struct TimingInfo {
  std::string name;
  RegionType type;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  std::chrono::nanoseconds duration;
  uint64_t id = 0;
};

struct EnergyTiming {
  // Default constructor
  EnergyTiming();

  EnergyTiming(uint64_t timing_id, RegionType type, std::string name);

  void end();

  bool is_ended() const;

  uint64_t get_duration_ms() const;

  uint64_t timing_id_;
  std::string name_;
  RegionType region_type_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time_;
};

struct EnergyTimer {
 public:
  void start_timing(uint64_t timing_id, RegionType type, std::string name);
  void end_timing(uint64_t timing_id);
  std::unordered_map<uint64_t, EnergyTiming>& get_timings();

 private:
  std::unordered_map<uint64_t, EnergyTiming> timings_;
};

// CSV Export functions for TimingInfo
namespace KokkosTools {
namespace Timer {
void export_kernels_csv(const std::deque<TimingInfo>& timings,
                        const std::string& filename);
void export_regions_csv(const std::deque<TimingInfo>& timings,
                        const std::string& filename);
void export_deepcopies_csv(const std::deque<TimingInfo>& timings,
                           const std::string& filename);
void print_kernels_summary(const std::deque<TimingInfo>& kernels);
void print_regions_summary(const std::deque<TimingInfo>& regions);
void print_deepcopies_summary(const std::deque<TimingInfo>& deepcopies);
}  // namespace Timer
}  // namespace KokkosTools