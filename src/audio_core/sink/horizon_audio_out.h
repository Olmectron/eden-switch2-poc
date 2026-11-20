// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>

// The console's audio output (libnx audout: 48 kHz, stereo, 16-bit). <switch.h> declares
// global names that collide with Eden's, so it is only included in the .cpp.
namespace AudioCore::Sink::HorizonAudioOut {

/// Frames accepted by each Play() call.
constexpr std::size_t FramesPerBuffer = 480;

/// Opens and starts the output. Returns false if the service is unavailable.
bool Open();
void Close();

/// Queues FramesPerBuffer interleaved stereo frames. Blocks while the device queue is
/// full, which paces the caller to real time.
void Play(const std::int16_t* samples);

} // namespace AudioCore::Sink::HorizonAudioOut
