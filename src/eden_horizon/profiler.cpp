// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eden_horizon/profiler.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>

#include <switch.h>

// Threads that named themselves (common/thread.cpp), kept in common/horizon/mman.cpp.
extern "C" {
struct eden_horizon_thread {
    char name[24];
    unsigned handle;
};
extern eden_horizon_thread eden_horizon_threads[];
extern int eden_horizon_thread_count;
extern char __start__[];
}

namespace Profiler {

namespace {

constexpr u64 SampleIntervalNs = 10'000'000;
// Samples are grouped by 64 bytes of code: close enough to a function's hot spot.
constexpr u64 BucketMask = ~u64{0x3F};
constexpr std::size_t TableSize = 8192;

struct Slot {
    u64 pc;
    u64 lr;
    u32 count;
    u16 thread;
    bool used;
};

std::array<Slot, TableSize> table{};
std::array<u32, 64> thread_samples{};
Mutex table_mutex;
Thread sampler_thread;
std::atomic<bool> running{false};

void Record(int thread, u64 pc, u64 lr) {
    const u64 bucket = pc & BucketMask;
    std::size_t index = (bucket >> 6) * 0x9E3779B97F4A7C15ULL >> 51;
    index = (index + static_cast<std::size_t>(thread) * 131) % TableSize;
    for (std::size_t probe = 0; probe < 64; ++probe) {
        Slot& slot = table[(index + probe) % TableSize];
        if (!slot.used) {
            slot = {bucket, lr, 1, static_cast<u16>(thread), true};
            return;
        }
        if (slot.pc == bucket && slot.thread == thread) {
            ++slot.count;
            return;
        }
    }
}

void SamplerMain(void*) {
    while (running.load(std::memory_order_relaxed)) {
        svcSleepThread(SampleIntervalNs);
        const int count = std::min(eden_horizon_thread_count, 64);
        for (int i = 0; i < count; ++i) {
            const Handle handle = eden_horizon_threads[i].handle;
            ThreadContext context;
            // Nothing here may take a lock the paused thread could hold: no allocation,
            // no logging, until it runs again.
            if (R_FAILED(svcSetThreadActivity(handle, ThreadActivity_Paused))) {
                continue;
            }
            const Result rc = svcGetThreadContext3(&context, handle);
            svcSetThreadActivity(handle, ThreadActivity_Runnable);
            if (R_FAILED(rc)) {
                continue;
            }
            mutexLock(&table_mutex);
            Record(i, context.pc.x, context.lr);
            ++thread_samples[static_cast<std::size_t>(i)];
            mutexUnlock(&table_mutex);
        }
    }
}

} // namespace

void Start() {
    if (running.exchange(true)) {
        return;
    }
    mutexInit(&table_mutex);
    if (R_FAILED(threadCreate(&sampler_thread, SamplerMain, nullptr, nullptr, 0x10000, 0x2C, -2)) ||
        R_FAILED(threadStart(&sampler_thread))) {
        running = false;
    }
}

void Stop() {
    if (!running.exchange(false)) {
        return;
    }
    threadWaitForExit(&sampler_thread);
    threadClose(&sampler_thread);
}

std::size_t TakeReport(std::span<Entry> out) {
    if (!running.load(std::memory_order_relaxed)) {
        return 0;
    }
    // Copy out and reset under the lock, sort outside it.
    static std::array<Slot, TableSize> snapshot;
    std::array<u32, 64> totals;
    mutexLock(&table_mutex);
    snapshot = table;
    totals = thread_samples;
    table.fill({});
    thread_samples.fill(0);
    mutexUnlock(&table_mutex);

    const auto last = std::partition(snapshot.begin(), snapshot.end(),
                                     [](const Slot& slot) { return slot.used; });
    std::sort(snapshot.begin(), last,
              [](const Slot& a, const Slot& b) { return a.count > b.count; });

    const u64 base = reinterpret_cast<u64>(__start__);
    constexpr u64 ModuleSpan = 0x8000000;
    std::size_t written = 0;
    for (auto it = snapshot.begin(); it != last && written < out.size(); ++it) {
        Entry& entry = out[written++];
        std::strncpy(entry.thread, eden_horizon_threads[it->thread].name, sizeof(entry.thread) - 1);
        entry.thread[sizeof(entry.thread) - 1] = '\0';
        entry.in_module = it->pc - base < ModuleSpan;
        entry.pc = entry.in_module ? it->pc - base : it->pc;
        entry.lr = it->lr - base < ModuleSpan ? it->lr - base : 0;
        entry.samples = it->count;
        entry.thread_samples = totals[it->thread];
    }
    return written;
}

} // namespace Profiler
