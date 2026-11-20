// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "audio_core/sink/sink.h"

namespace Core {
class System;
}

namespace AudioCore::Sink {
class SinkStream;

/// Audio sink for Horizon hosts (libnx audout). The device takes a single stream, so
/// one thread mixes every output stream of the sink into it.
class HorizonSink final : public Sink {
public:
    explicit HorizonSink(std::string_view device_id);
    ~HorizonSink() override;

    SinkStream* AcquireSinkStream(Core::System& system, u32 system_channels,
                                  const std::string& name, StreamType type) override;
    void CloseStream(SinkStream* stream) override;
    void CloseStreams() override;
    f32 GetDeviceVolume() const override;
    void SetDeviceVolume(f32 volume) override;
    void SetSystemVolume(f32 volume) override;

private:
    void MixerThread(std::stop_token stop_token);

    std::mutex streams_mutex;
    std::vector<SinkStreamPtr> sink_streams;
    std::jthread mixer_thread;
};

std::vector<std::string> ListHorizonSinkDevices(bool capture);
u32 GetHorizonLatency();

} // namespace AudioCore::Sink
