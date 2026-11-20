// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Dynarmic::Backend::Arm64 {

/// Code memory for Horizon hosts, with the interface of oaknut::CodeBlock.
///
/// Horizon never maps memory writable and executable at once. libnx's Jit gives an
/// executable view (ptr) and a separate writable view (wptr). On Switch 2 (kaiHEN) the
/// writable view is a staging buffer: nothing written there runs until invalidate()
/// publishes it to the executable view.
class HorizonCodeBlock {
public:
    explicit HorizonCodeBlock(std::size_t size);
    ~HorizonCodeBlock();

    HorizonCodeBlock(const HorizonCodeBlock&) = delete;
    HorizonCodeBlock& operator=(const HorizonCodeBlock&) = delete;
    HorizonCodeBlock(HorizonCodeBlock&&) = delete;
    HorizonCodeBlock& operator=(HorizonCodeBlock&&) = delete;

    /// Executable view. Emitted code runs, and is addressed, here.
    std::uint32_t* ptr() const {
        return m_rx;
    }

    /// Writable view at the same offsets as ptr().
    std::uint32_t* wptr() const {
        return m_rw;
    }

    /// Between unprotect() and the matching protect() (they nest), invalidate() only notes
    /// the range; the outermost protect() then publishes them all at once. Each publication is a round trip to another
    /// process, and emitting one block invalidates every block that links to it.
    void unprotect();
    void protect();

    /// Publishes [mem, mem + size) of the executable view from the writable view.
    void invalidate(std::uint32_t* mem, std::size_t size);
    void invalidate_all();

private:
    // Keeps <switch.h> and its global names out of this header.
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::uint32_t* m_rx = nullptr;
    std::uint32_t* m_rw = nullptr;
    std::size_t m_size = 0;

    struct Range {
        std::uint64_t offset;
        std::uint64_t size;
    };
    void Publish(const Range* ranges, std::size_t count);

    std::vector<Range> m_pending;
    int m_batch_depth = 0;
};

} // namespace Dynarmic::Backend::Arm64
