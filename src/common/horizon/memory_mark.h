// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/logging.h"

// libounce's allocator figures (switch2/memory/heap.h), declared here because
// <switch.h> cannot be included next to Eden's headers.
extern "C" {
u64 heapGetTotalSize(void);
u64 heapGetFreeSize(void);
}

/// Bring-up aid for Horizon hosts: logs how much of the heap is in use at this point,
/// to find which step of startup takes it.
#define HORIZON_MEMORY_MARK(where)                                                                 \
    LOG_INFO(Core, "Memory mark: {} ({} MiB in use)", where,                                       \
             (heapGetTotalSize() - heapGetFreeSize()) >> 20)
