// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio_core/sink/horizon_audio_out.h"

#include <array>
#include <cstdlib>
#include <cstring>

#include <switch.h>

namespace AudioCore::Sink::HorizonAudioOut {

namespace {

constexpr std::size_t Channels = 2;
constexpr std::size_t DataSize = FramesPerBuffer * Channels * sizeof(s16);
// audout wants page-aligned buffers of page-aligned size.
constexpr std::size_t BufferSize = (DataSize + 0xFFF) & ~std::size_t{0xFFF};
// Buffers kept queued in the device: about 40 ms of latency.
constexpr std::size_t QueueDepth = 4;

std::array<AudioOutBuffer, QueueDepth> buffers{};
std::size_t next_buffer = 0;
std::size_t queued = 0;
bool is_open = false;

} // namespace

bool Open() {
    if (is_open) {
        return true;
    }
    if (R_FAILED(audoutInitialize())) {
        return false;
    }
    for (AudioOutBuffer& buffer : buffers) {
        buffer = {};
        buffer.buffer = std::aligned_alloc(0x1000, BufferSize);
        buffer.buffer_size = BufferSize;
        buffer.data_size = DataSize;
        if (buffer.buffer == nullptr) {
            Close();
            return false;
        }
        std::memset(buffer.buffer, 0, BufferSize);
    }
    if (R_FAILED(audoutStartAudioOut())) {
        Close();
        return false;
    }
    next_buffer = 0;
    queued = 0;
    is_open = true;
    return true;
}

void Close() {
    if (is_open) {
        audoutStopAudioOut();
    }
    audoutExit();
    for (AudioOutBuffer& buffer : buffers) {
        std::free(buffer.buffer);
        buffer.buffer = nullptr;
    }
    is_open = false;
}

void Play(const std::int16_t* samples) {
    if (!is_open) {
        return;
    }
    while (queued == QueueDepth) {
        AudioOutBuffer* released = nullptr;
        u32 released_count = 0;
        // Buffers are released in the order they were queued, so the count is enough.
        if (R_FAILED(audoutWaitPlayFinish(&released, &released_count, 1'000'000'000ULL))) {
            return;
        }
        queued -= released_count < queued ? released_count : queued;
    }

    AudioOutBuffer& buffer = buffers[next_buffer];
    std::memcpy(buffer.buffer, samples, DataSize);
    buffer.data_offset = 0;
    if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&buffer))) {
        next_buffer = (next_buffer + 1) % QueueDepth;
        ++queued;
    }
}

} // namespace AudioCore::Sink::HorizonAudioOut
