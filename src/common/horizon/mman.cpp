// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <pwd.h>
#include <sys/mman.h>
#include <unistd.h>

namespace {
constexpr size_t PageSize = 0x1000;
constexpr size_t AlignUpPage(size_t size) {
    return (size + PageSize - 1) & ~(PageSize - 1);
}
} // namespace

extern "C" {

// The big mappings made so far, for the frontend's memory log (common/horizon/stats.h).
constexpr size_t LargeMapping = 8 * 1024 * 1024;
constexpr int MaxLargeMappings = 64;
unsigned long long eden_horizon_large_mmaps[MaxLargeMappings];
int eden_horizon_large_mmap_count = 0;

// TODO: large sparse reservations (guest page tables) are committed in full here.
// Reserve address space with virtmem and map heap pages on demand instead.
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (fd != -1 || !(flags & MAP_ANONYMOUS) || (flags & MAP_FIXED) || length == 0) {
        errno = ENOTSUP;
        return MAP_FAILED;
    }
    void* base = std::aligned_alloc(PageSize, AlignUpPage(length));
    if (base == nullptr) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    std::memset(base, 0, AlignUpPage(length));
    if (length >= LargeMapping) {
        const int index = __atomic_fetch_add(&eden_horizon_large_mmap_count, 1, __ATOMIC_RELAXED);
        if (index < MaxLargeMappings) {
            eden_horizon_large_mmaps[index] = length;
        }
    }
    return base;
}

int munmap(void* addr, size_t) {
    std::free(addr);
    return 0;
}

// Everything handed out by mmap() above is already readable and writable.
int mprotect(void*, size_t, int) {
    return 0;
}

int madvise(void*, size_t, int) {
    return 0;
}

int posix_madvise(void*, size_t, int) {
    return 0;
}

// Declared by newlib but not implemented by libnx.
int pipe(int[2]) {
    errno = ENOSYS;
    return -1;
}

struct passwd* getpwuid(uid_t) {
    return nullptr;
}

// newlib's pthread_detach() hook, which libnx leaves unsupported (ENOSYS), so
// std::thread::detach() throws. Accept the detach and let the thread run to its end.
// TODO: the thread's stack and handle are never reclaimed; free them when it exits.
struct __pthread_t;
int __syscall_thread_detach(struct __pthread_t*) {
    return 0;
}

// For OpenSSL's CPU feature detection (see the top-level CMakeLists.txt). The Switch 2's
// Cortex-A78C has NEON, AES, PMULL, SHA-1 and SHA-256; bits as in Linux's AT_HWCAP.
unsigned long getauxval(unsigned long type) {
    constexpr unsigned long AtHwcap = 16;
    constexpr unsigned long Neon = 1UL << 1, Aes = 1UL << 3, Pmull = 1UL << 4, Sha1 = 1UL << 5,
                            Sha256 = 1UL << 6;
    return type == AtHwcap ? (Neon | Aes | Pmull | Sha1 | Sha256) : 0;
}

// See common/horizon/stats.h.
unsigned long long eden_horizon_texture_bytes = 0;
unsigned long long eden_horizon_buffer_bytes = 0;
unsigned long long eden_horizon_jit_blocks = 0;
unsigned long long eden_horizon_jit_cache_clears = 0;
unsigned long long eden_horizon_jit_heap_clears = 0;
unsigned long long eden_horizon_jit_compile_micros = 0;
int eden_horizon_compact_address_space = 0;

// The emulator's named threads, for the frontend's profiler (see common/thread.cpp).
struct eden_horizon_thread {
    char name[24];
    unsigned handle;
};
eden_horizon_thread eden_horizon_threads[64];
int eden_horizon_thread_count = 0;

void eden_horizon_register_thread(const char* name, unsigned handle) {
    const int index = __atomic_fetch_add(&eden_horizon_thread_count, 1, __ATOMIC_RELAXED);
    if (index >= 64) {
        __atomic_store_n(&eden_horizon_thread_count, 64, __ATOMIC_RELAXED);
        return;
    }
    std::strncpy(eden_horizon_threads[index].name, name, sizeof(eden_horizon_threads[index].name) - 1);
    eden_horizon_threads[index].handle = handle;
}

} // extern "C"
