// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dynarmic/backend/arm64/horizon_code_block.h"

#include <cstdio>
#include <stdexcept>

#include <switch.h>
#ifdef __SWITCH2__
#    include <kaihen.h>
#endif

// Totals of code publication, for the frontend's performance log: each one is a round
// trip to another process, so it is the first suspect when new code runs slowly.
extern "C" {
unsigned long long eden_horizon_jit_publish_count = 0;
unsigned long long eden_horizon_jit_publish_bytes = 0;
unsigned long long eden_horizon_jit_publish_micros = 0;
}

namespace Dynarmic::Backend::Arm64 {

struct HorizonCodeBlock::Impl {
    Jit jit{};
};

HorizonCodeBlock::HorizonCodeBlock(std::size_t size)
    : m_impl{std::make_unique<Impl>()}, m_size{size} {
#ifdef __SWITCH2__
    if (const Result rc = jitCreate(&m_impl->jit, size); R_FAILED(rc)) {
        char message[96];
        std::snprintf(message, sizeof(message), "jitCreate(%zu MiB) failed: 0x%x (module %u, description %u)",
                      size >> 20, rc, R_MODULE(rc), R_DESCRIPTION(rc));
        throw std::runtime_error{message};
    }
    m_rx = static_cast<std::uint32_t*>(jitGetRxAddr(&m_impl->jit));
    m_rw = static_cast<std::uint32_t*>(jitGetRwAddr(&m_impl->jit));
#else
    // libnx on Switch 1 exposes one view at a time; this backend needs both.
    throw std::runtime_error{"JIT needs Switch 2 (kaiHEN) code memory"};
#endif
}

HorizonCodeBlock::~HorizonCodeBlock() {
    if (m_rx != nullptr)
        jitClose(&m_impl->jit);
}

void HorizonCodeBlock::Publish(const Range* ranges, std::size_t count) {
#ifdef __SWITCH2__
    static_assert(sizeof(Range) == sizeof(KaihenJitRange));
    // Also invalidates the instruction cache for the ranges.
    const u64 start = armGetSystemTick();
    const Result rc = kaihenJitPublishRanges(
        &m_impl->jit, reinterpret_cast<const KaihenJitRange*>(ranges), count);
    u64 bytes = 0;
    for (std::size_t i = 0; i < count; ++i) {
        bytes += ranges[i].size;
    }
    __atomic_fetch_add(&eden_horizon_jit_publish_count, 1, __ATOMIC_RELAXED);
    __atomic_fetch_add(&eden_horizon_jit_publish_bytes, bytes, __ATOMIC_RELAXED);
    __atomic_fetch_add(&eden_horizon_jit_publish_micros,
                       armTicksToNs(armGetSystemTick() - start) / 1000, __ATOMIC_RELAXED);
    if (R_FAILED(rc))
        diagAbortWithResult(rc);
#else
    (void)ranges;
    (void)count;
#endif
}

void HorizonCodeBlock::unprotect() {
    ++m_batch_depth;
}

void HorizonCodeBlock::protect() {
    if (m_batch_depth > 0 && --m_batch_depth > 0) {
        return;
    }
    if (!m_pending.empty()) {
        Publish(m_pending.data(), m_pending.size());
        m_pending.clear();
    }
}

void HorizonCodeBlock::invalidate(std::uint32_t* mem, std::size_t size) {
    if (size == 0) {
        return;
    }
    const Range range{
        .offset = reinterpret_cast<std::uintptr_t>(mem) - reinterpret_cast<std::uintptr_t>(m_rx),
        .size = size,
    };
    if (m_batch_depth > 0) {
        m_pending.push_back(range);
    } else {
        Publish(&range, 1);
    }
}

void HorizonCodeBlock::invalidate_all() {
    invalidate(m_rx, m_size);
}

} // namespace Dynarmic::Backend::Arm64
