// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <span>

#include "audio_core/common/common.h"
#include "audio_core/sink/horizon_audio_out.h"
#include "audio_core/sink/horizon_sink.h"
#include "audio_core/sink/sink_stream.h"
#include "common/logging.h"
#include "common/thread.h"
#include "core/core.h"

namespace AudioCore::Sink {

namespace {

constexpr u32 DeviceChannels = 2;
constexpr std::size_t SamplesPerBuffer = HorizonAudioOut::FramesPerBuffer * DeviceChannels;

class HorizonSinkStream final : public SinkStream {
public:
    HorizonSinkStream(u32 system_channels_, StreamType type_, Core::System& system_)
        : SinkStream{system_, type_} {
        system_channels = system_channels_;
        device_channels = DeviceChannels;
    }

    ~HorizonSinkStream() override {
        Finalize();
    }

    void Finalize() override {
        Stop();
    }

    void Start(bool resume = false) override {
        paused = false;
    }

    void Stop() override {
        if (!paused) {
            SignalPause();
        }
    }

    [[nodiscard]] bool IsOutput() const {
        return type != StreamType::In;
    }
};

} // Anonymous namespace

HorizonSink::HorizonSink(std::string_view) {
    device_channels = DeviceChannels;
}

HorizonSink::~HorizonSink() {
    mixer_thread = {};
    CloseStreams();
}

SinkStream* HorizonSink::AcquireSinkStream(Core::System& system, u32 system_channels_,
                                           const std::string&, StreamType type) {
    system_channels = system_channels_;
    std::scoped_lock lock{streams_mutex};
    SinkStreamPtr& stream = sink_streams.emplace_back(
        std::make_unique<HorizonSinkStream>(system_channels, type, system));
    stream->SetDeviceVolume(device_volume);

    // The device is opened by the first output stream; capture is not supported.
    if (type != StreamType::In && !mixer_thread.joinable()) {
        mixer_thread = std::jthread([this](std::stop_token token) { MixerThread(token); });
    }
    return stream.get();
}

void HorizonSink::MixerThread(std::stop_token stop_token) {
    Common::SetCurrentThreadName("HorizonAudio");
    if (!HorizonAudioOut::Open()) {
        LOG_ERROR(Audio_Sink, "Could not open the console's audio output");
        return;
    }
    LOG_INFO(Audio_Sink, "Console audio output opened: 48000 Hz, {} frames per buffer",
             HorizonAudioOut::FramesPerBuffer);

    std::array<s16, SamplesPerBuffer> stream_samples;
    std::array<s32, SamplesPerBuffer> mix;
    std::array<s16, SamplesPerBuffer> output;

    while (!stop_token.stop_requested()) {
        mix.fill(0);
        {
            std::scoped_lock lock{streams_mutex};
            for (auto& stream : sink_streams) {
                auto& horizon_stream = static_cast<HorizonSinkStream&>(*stream);
                if (!horizon_stream.IsOutput() || horizon_stream.IsPaused()) {
                    continue;
                }
                stream_samples.fill(0);
                horizon_stream.ProcessAudioOutAndRender(stream_samples,
                                                        HorizonAudioOut::FramesPerBuffer);
                for (std::size_t i = 0; i < SamplesPerBuffer; ++i) {
                    mix[i] += stream_samples[i];
                }
            }
        }
        for (std::size_t i = 0; i < SamplesPerBuffer; ++i) {
            output[i] = static_cast<s16>(std::clamp<s32>(mix[i], -32768, 32767));
        }
        // Blocks until the device has room, pacing this loop to real time.
        HorizonAudioOut::Play(output.data());
    }
    HorizonAudioOut::Close();
}

void HorizonSink::CloseStream(SinkStream* stream) {
    std::scoped_lock lock{streams_mutex};
    std::erase_if(sink_streams, [stream](const SinkStreamPtr& s) { return s.get() == stream; });
}

void HorizonSink::CloseStreams() {
    std::scoped_lock lock{streams_mutex};
    sink_streams.clear();
}

f32 HorizonSink::GetDeviceVolume() const {
    return device_volume;
}

void HorizonSink::SetDeviceVolume(f32 volume) {
    device_volume = volume;
    std::scoped_lock lock{streams_mutex};
    for (auto& stream : sink_streams) {
        stream->SetDeviceVolume(volume);
    }
}

void HorizonSink::SetSystemVolume(f32 volume) {
    std::scoped_lock lock{streams_mutex};
    for (auto& stream : sink_streams) {
        stream->SetSystemVolume(volume);
    }
}

std::vector<std::string> ListHorizonSinkDevices(bool capture) {
    if (capture) {
        return {};
    }
    return {"Console"};
}

u32 GetHorizonLatency() {
    return TargetSampleCount * 2;
}

} // namespace AudioCore::Sink
