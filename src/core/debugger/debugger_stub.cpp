// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// GDB stub server without a network backend, for hosts where Boost.Asio is
// unavailable (Horizon). The debugger never attaches.

#include "core/debugger/debugger.h"

namespace Core {

class DebuggerImpl {};

Debugger::Debugger(Core::System&, u16) {}

Debugger::~Debugger() = default;

bool Debugger::NotifyThreadStopped(Kernel::KThread*) {
    return false;
}

void Debugger::NotifyShutdown() {}

bool Debugger::NotifyThreadWatchpoint(Kernel::KThread*, const Kernel::DebugWatchpoint&) {
    return false;
}

} // namespace Core
