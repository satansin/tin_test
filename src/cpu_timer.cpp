#include "tin_gen/cpu_timer.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace tin_gen {
namespace {

std::int64_t wall_time_nanoseconds() {
#if defined(_WIN32)
  static LARGE_INTEGER frequency = {};
  if (frequency.QuadPart == 0) {
    if (!QueryPerformanceFrequency(&frequency)) {
      throw std::runtime_error("QueryPerformanceFrequency failed.");
    }
  }
  LARGE_INTEGER counter{};
  if (!QueryPerformanceCounter(&counter)) {
    throw std::runtime_error("QueryPerformanceCounter failed.");
  }
  return static_cast<std::int64_t>(counter.QuadPart * 1'000'000'000LL / frequency.QuadPart);
#elif defined(CLOCK_MONOTONIC)
  timespec ts{};
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    throw std::runtime_error("clock_gettime(CLOCK_MONOTONIC) failed.");
  }
  return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL +
         static_cast<std::int64_t>(ts.tv_nsec);
#else
  timespec ts{};
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
    throw std::runtime_error("clock_gettime(CLOCK_REALTIME) failed.");
  }
  return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL +
         static_cast<std::int64_t>(ts.tv_nsec);
#endif
}

std::int64_t cpu_time_nanoseconds() {
#if defined(_WIN32)
  FILETIME creation{};
  FILETIME exit{};
  FILETIME kernel{};
  FILETIME user{};
  if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
    throw std::runtime_error("GetProcessTimes failed.");
  }
  ULARGE_INTEGER kernel_time{};
  ULARGE_INTEGER user_time{};
  kernel_time.LowPart = kernel.dwLowDateTime;
  kernel_time.HighPart = kernel.dwHighDateTime;
  user_time.LowPart = user.dwLowDateTime;
  user_time.HighPart = user.dwHighDateTime;
  constexpr std::int64_t k100ns_per_ns = 100;
  return static_cast<std::int64_t>((kernel_time.QuadPart + user_time.QuadPart) * k100ns_per_ns);
#elif defined(CLOCK_PROCESS_CPUTIME_ID)
  timespec ts{};
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
    throw std::runtime_error("clock_gettime(CLOCK_PROCESS_CPUTIME_ID) failed.");
  }
  return static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL +
         static_cast<std::int64_t>(ts.tv_nsec);
#else
  if (CLOCKS_PER_SEC <= 0) {
    throw std::runtime_error("Invalid CLOCKS_PER_SEC.");
  }
  return static_cast<std::int64_t>(std::clock()) * 1'000'000'000LL / CLOCKS_PER_SEC;
#endif
}

constexpr int kSecondsPrecision = 4;
constexpr int kSubSecondPrecision = 9;

int sub_minute_seconds_precision(const double total_seconds) {
  return total_seconds < 1.0 ? kSubSecondPrecision : kSecondsPrecision;
}

void append_seconds_value(std::ostream& out, const double seconds, const int precision) {
  std::ostringstream buf;
  buf << std::fixed << std::setprecision(precision) << seconds;
  std::string text = buf.str();
  if (precision > kSecondsPrecision) {
    if (const std::size_t dot = text.find('.'); dot != std::string::npos) {
      while (text.size() > dot + 1 && text.back() == '0') {
        text.pop_back();
      }
    }
  }
  out << text;
}

}  // namespace

void append_formatted_elapsed_seconds(std::ostream& out, const double total_seconds) {
  double t = total_seconds < 0.0 ? 0.0 : total_seconds;

  constexpr double kSecPerMin = 60.0;
  constexpr double kSecPerHour = 3600.0;
  constexpr double kSecPerDay = 86400.0;

  if (t < kSecPerMin) {
    append_seconds_value(out, t, sub_minute_seconds_precision(t));
    out << " s";
    return;
  }
  if (t < kSecPerHour) {
    const auto minutes = static_cast<long long>(t / kSecPerMin);
    const double seconds = t - static_cast<double>(minutes) * kSecPerMin;
    out << minutes << " m ";
    append_seconds_value(out, seconds, kSecondsPrecision);
    out << " s";
    return;
  }
  if (t < kSecPerDay) {
    const auto hours = static_cast<long long>(t / kSecPerHour);
    t -= static_cast<double>(hours) * kSecPerHour;
    const auto minutes = static_cast<long long>(t / kSecPerMin);
    const double seconds = t - static_cast<double>(minutes) * kSecPerMin;
    out << hours << " h " << minutes << " m ";
    append_seconds_value(out, seconds, kSecondsPrecision);
    out << " s";
    return;
  }

  const auto days = static_cast<long long>(t / kSecPerDay);
  t -= static_cast<double>(days) * kSecPerDay;
  const auto hours = static_cast<long long>(t / kSecPerHour);
  t -= static_cast<double>(hours) * kSecPerHour;
  const auto minutes = static_cast<long long>(t / kSecPerMin);
  const double seconds = t - static_cast<double>(minutes) * kSecPerMin;
  out << days << " d " << hours << " h " << minutes << " m ";
  append_seconds_value(out, seconds, kSecondsPrecision);
  out << " s";
}

std::string format_elapsed_seconds(const double total_seconds) {
  std::ostringstream out;
  append_formatted_elapsed_seconds(out, total_seconds);
  return out.str();
}

void WallTimer::start() {
  start_ns_ = wall_time_nanoseconds();
  end_ns_.reset();
}

void WallTimer::stop() {
  if (!start_ns_) {
    throw std::runtime_error("WallTimer::stop() called without start().");
  }
  end_ns_ = wall_time_nanoseconds();
}

bool WallTimer::is_running() const { return start_ns_.has_value() && !end_ns_.has_value(); }

std::int64_t WallTimer::elapsed_nanoseconds() const {
  if (!start_ns_) {
    return 0;
  }
  const std::int64_t end = end_ns_.value_or(wall_time_nanoseconds());
  return end - *start_ns_;
}

double WallTimer::elapsed_seconds() const {
  return static_cast<double>(elapsed_nanoseconds()) / 1e9;
}

double WallTimer::elapsed_milliseconds() const {
  return static_cast<double>(elapsed_nanoseconds()) / 1e6;
}

void CpuTimer::start() {
  start_ns_ = cpu_time_nanoseconds();
  end_ns_.reset();
}

void CpuTimer::stop() {
  if (!start_ns_) {
    throw std::runtime_error("CpuTimer::stop() called without start().");
  }
  end_ns_ = cpu_time_nanoseconds();
}

bool CpuTimer::is_running() const { return start_ns_.has_value() && !end_ns_.has_value(); }

std::int64_t CpuTimer::elapsed_nanoseconds() const {
  if (!start_ns_) {
    return 0;
  }
  const std::int64_t end = end_ns_.value_or(cpu_time_nanoseconds());
  return end - *start_ns_;
}

double CpuTimer::elapsed_seconds() const {
  return static_cast<double>(elapsed_nanoseconds()) / 1e9;
}

double CpuTimer::elapsed_milliseconds() const {
  return static_cast<double>(elapsed_nanoseconds()) / 1e6;
}

void print_cpu_wall_timing(const std::string_view label, const CpuTimer& cpu,
                           const WallTimer& wall) {
  std::cout << label << " timing:\n  CPU time: ";
  append_formatted_elapsed_seconds(std::cout, cpu.elapsed_seconds());
  std::cout << "\n  Wall time: ";
  append_formatted_elapsed_seconds(std::cout, wall.elapsed_seconds());
  std::cout << '\n';
}

CpuTimerReport::CpuTimerReport(std::string label) : label_(std::move(label)) { timer_.start(); }

CpuTimerReport::~CpuTimerReport() {
  if (reported_) {
    return;
  }
  if (timer_.is_running()) {
    timer_.stop();
  }
  std::cout << label_ << " CPU time: ";
  append_formatted_elapsed_seconds(std::cout, timer_.elapsed_seconds());
  std::cout << '\n';
}

CpuTimer& CpuTimerReport::timer() { return timer_; }

}  // namespace tin_gen
