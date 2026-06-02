#pragma once

namespace tin_gen {

/// Line-buffer stdout and unbuffer stdout/stderr C++ streams so progress logs appear
/// immediately when stdout is not a TTY (batch jobs, pipes, log files).
void configure_immediate_console_output();

}  // namespace tin_gen
