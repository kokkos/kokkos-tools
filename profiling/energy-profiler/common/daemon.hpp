#pragma once

#include <functional>
#include <thread>
#include <chrono>

class Daemon {
 public:
  Daemon(std::function<void()> func, int interval_ms)
      : func_(func), interval_(interval_ms) {};

  void start();
  void tick();
  void stop();
  bool is_running() const { return running_; }
  std::thread& get_thread() { return thread_; }

 private:
  std::chrono::milliseconds interval_;
  bool running_{false};
  std::function<void()> func_;
  std::thread thread_;
};