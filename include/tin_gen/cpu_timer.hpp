#pragma once

#include <cstdint>
#include <iosfwd>
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

/// Format elapsed seconds for display: under 1m as `s`, then `m s`, `h m s`, `d h m s`.
/// Sub-second values use up to 9 fractional digits; otherwise seconds use 4 decimal places.
[[nodiscard]] std::string format_elapsed_seconds(double seconds);

/// Append @ref format_elapsed_seconds() to @p out.
void append_formatted_elapsed_seconds(std::ostream& out, double seconds);

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
