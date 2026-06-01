#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace tin_gen {

/// Monotonic wall-clock elapsed time (real time, not CPU).
class WallTimer {
 public:
  void start();
  void stop();

  [[nodiscard]] bool is_running() const;
  [[nodiscard]] std::int64_t elapsed_nanoseconds() const;
  [[nodiscard]] double elapsed_seconds() const;
  [[nodiscard]] double elapsed_milliseconds() const;

 private:
  std::optional<std::int64_t> start_ns_;
  std::optional<std::int64_t> end_ns_;
};

/// Process CPU time (not wall clock).
class CpuTimer {
 public:
  void start();
  void stop();

  [[nodiscard]] bool is_running() const;
  [[nodiscard]] std::int64_t elapsed_nanoseconds() const;
  [[nodiscard]] double elapsed_seconds() const;
  [[nodiscard]] double elapsed_milliseconds() const;

 private:
  std::optional<std::int64_t> start_ns_;
  std::optional<std::int64_t> end_ns_;
};

/// Print elapsed CPU and wall time to stdout.
void print_cpu_wall_timing(std::string_view label, const CpuTimer& cpu, const WallTimer& wall);

/// Starts on construction; stops and prints elapsed CPU time on destruction.
class CpuTimerReport {
 public:
  explicit CpuTimerReport(std::string label);
  ~CpuTimerReport();

  CpuTimerReport(const CpuTimerReport&) = delete;
  CpuTimerReport& operator=(const CpuTimerReport&) = delete;

  [[nodiscard]] CpuTimer& timer();

 private:
  std::string label_;
  CpuTimer timer_;
  bool reported_ = false;
};

}  // namespace tin_gen
