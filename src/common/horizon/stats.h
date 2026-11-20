// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Figures the Horizon frontend logs to see where memory goes on a host where one
// 9 GiB heap holds everything. Plain C globals so that dynarmic, video_core and the
// frontend can share them without depending on each other. Defined in mman.cpp.
extern "C" {
extern unsigned long long eden_horizon_texture_bytes; ///< Texture cache's own estimate
extern unsigned long long eden_horizon_buffer_bytes;  ///< Buffer cache's own estimate
extern unsigned long long eden_horizon_jit_blocks;    ///< Compiled blocks alive, all cores
extern unsigned long long eden_horizon_jit_cache_clears;
extern unsigned long long eden_horizon_jit_heap_clears;     ///< Those made to free heap
extern unsigned long long eden_horizon_jit_compile_micros;  ///< Compiling and publishing
/// Sizes in bytes of the mappings of 8 MiB and more made through the mmap() shim
extern unsigned long long eden_horizon_large_mmaps[64];
extern int eden_horizon_large_mmap_count;
/// Set by the frontend: nonzero gives 39-bit programs a 36-bit address space
extern int eden_horizon_compact_address_space;
}
