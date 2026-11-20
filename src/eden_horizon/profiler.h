// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

// Sampling profiler for a host with no debugger and no signals: a thread pauses each
// named emulator thread 100 times a second and notes where it was. Addresses are
// offsets into eden-horizon.elf, to resolve with addr2line.
namespace Profiler {

struct Entry {
    char thread[24];
    bool in_module;             ///< false: generated code or another module; pc is absolute
    std::uint64_t pc;           ///< Start of the 64 bytes of code the samples fell in
    std::uint64_t lr;           ///< A return address seen there, 0 if outside the module
    std::uint32_t samples;
    std::uint32_t thread_samples; ///< All samples of that thread in the period
};

void Start();
void Stop();

/// Writes the busiest places since the last call, most samples first, and starts a new
/// period. Returns how many entries were written.
std::size_t TakeReport(std::span<Entry> out);

} // namespace Profiler
