// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Cemuhook UDP input without a network backend, for hosts where Boost.Asio is
// unavailable (Horizon). The engine registers but never reports devices.

#include "input_common/drivers/udp_client.h"

namespace InputCommon::CemuhookUDP {

class Socket {};

UDPClient::UDPClient(std::string input_engine_) : InputEngine(std::move(input_engine_)) {}

UDPClient::~UDPClient() = default;

UDPClient::ClientConnection::ClientConnection() = default;

UDPClient::ClientConnection::~ClientConnection() = default;

void UDPClient::ReloadSockets() {}

std::vector<Common::ParamPackage> UDPClient::GetInputDevices() const {
    return {};
}

ButtonMapping UDPClient::GetButtonMappingForDevice(const Common::ParamPackage&) {
    return {};
}

AnalogMapping UDPClient::GetAnalogMappingForDevice(const Common::ParamPackage&) {
    return {};
}

MotionMapping UDPClient::GetMotionMappingForDevice(const Common::ParamPackage&) {
    return {};
}

Common::Input::ButtonNames UDPClient::GetUIName(const Common::ParamPackage&) const {
    return Common::Input::ButtonNames::Invalid;
}

bool UDPClient::IsStickInverted(const Common::ParamPackage&) {
    return false;
}

void TestCommunication(const std::string&, u16, const std::function<void()>&,
                       const std::function<void()>& failure_callback) {
    failure_callback();
}

CalibrationConfigurationJob::CalibrationConfigurationJob(const std::string&, u16,
                                                         std::function<void(Status)>,
                                                         std::function<void(u16, u16, u16, u16)>) {}

CalibrationConfigurationJob::~CalibrationConfigurationJob() = default;

void CalibrationConfigurationJob::Stop() {}

} // namespace InputCommon::CemuhookUDP
