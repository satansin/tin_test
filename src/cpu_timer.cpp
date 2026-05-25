#include "tin_gen/cpu_timer.hpp"

#include <ctime>
#include <iostream>
#include <stdexcept>

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

}  // namespace

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

CpuTimerReport::CpuTimerReport(std::string label) : label_(std::move(label)) { timer_.start(); }

CpuTimerReport::~CpuTimerReport() {
  if (reported_) {
    return;
  }
  if (timer_.is_running()) {
    timer_.stop();
  }
  std::cout << label_ << " CPU time: " << timer_.elapsed_milliseconds() << " ms\n";
}

CpuTimer& CpuTimerReport::timer() { return timer_; }

}  // namespace tin_gen
