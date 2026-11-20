/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mutex>

#include <oaknut/code_block.hpp>
#include <oaknut/oaknut.hpp>

#include "dynarmic/backend/arm64/abi.h"
#include "dynarmic/common/spin_lock.h"

namespace Dynarmic {

using Backend::Arm64::Wscratch0;
using Backend::Arm64::Wscratch1;
using namespace oaknut::util;

void EmitSpinLockLock(oaknut::CodeGenerator& code, oaknut::XReg ptr) {
    oaknut::Label start, loop;

    code.MOV(Wscratch1, 1);
    code.SEVL();
    code.l(start);
    code.WFE();
    code.l(loop);
    code.LDAXR(Wscratch0, ptr);
    code.CBNZ(Wscratch0, start);
    code.STXR(Wscratch0, Wscratch1, ptr);
    code.CBNZ(Wscratch0, loop);
}

void EmitSpinLockUnlock(oaknut::CodeGenerator& code, oaknut::XReg ptr) {
    code.STLR(WZR, ptr);
}

#ifdef __SWITCH__

// Executable memory is scarce and costly to write on Horizon hosts, and the host is
// already arm64: take the lock with compiler builtins instead of generated code.
//
// Without fastmem every guest exclusive access (LDXR/STXR) goes through the exclusive
// monitor under this one lock, so a guest thread spinning on an atomic takes it
// thousands of times a frame. A test-and-set lock lets that thread win it again each
// time and starves the other cores; tickets hand it over in arrival order.
// `storage` holds the next ticket in its high half and the one being served in its low.
void SpinLock::Lock() noexcept {
    constexpr unsigned TicketUnit = 0x10000;
    constexpr unsigned ServingMask = 0xFFFF;
    auto* const state = const_cast<unsigned*>(reinterpret_cast<volatile unsigned*>(&storage));
    const unsigned ticket = __atomic_fetch_add(state, TicketUnit, __ATOMIC_ACQUIRE) >> 16;
    while ((__atomic_load_n(state, __ATOMIC_ACQUIRE) & ServingMask) != ticket) {
        __asm__ volatile("yield");
    }
}

void SpinLock::Unlock() noexcept {
    constexpr unsigned ServingMask = 0xFFFF;
    auto* const state = const_cast<unsigned*>(reinterpret_cast<volatile unsigned*>(&storage));
    // Only the holder advances the served ticket, but others add tickets meanwhile.
    unsigned current = __atomic_load_n(state, __ATOMIC_RELAXED);
    unsigned next;
    do {
        next = (current & ~ServingMask) | ((current + 1) & ServingMask);
    } while (!__atomic_compare_exchange_n(state, &current, next, true, __ATOMIC_RELEASE,
                                          __ATOMIC_RELAXED));
}

#else

namespace {

struct SpinLockImpl {
    SpinLockImpl();

    void Initialize();

    oaknut::CodeBlock mem;
    oaknut::CodeGenerator code;

    void (*lock)(volatile int*);
    void (*unlock)(volatile int*);
};

std::once_flag flag;
SpinLockImpl impl;

SpinLockImpl::SpinLockImpl()
        : mem{4096}
        , code{mem.ptr(), mem.ptr()} {}

void SpinLockImpl::Initialize() {
    mem.unprotect();

    lock = code.xptr<void (*)(volatile int*)>();
    EmitSpinLockLock(code, X0);
    code.RET();

    unlock = code.xptr<void (*)(volatile int*)>();
    EmitSpinLockUnlock(code, X0);
    code.RET();

    mem.protect();
    mem.invalidate_all();
}

}  // namespace

void SpinLock::Lock() noexcept {
    std::call_once(flag, &SpinLockImpl::Initialize, impl);
    impl.lock(&storage);
}

void SpinLock::Unlock() noexcept {
    std::call_once(flag, &SpinLockImpl::Initialize, impl);
    impl.unlock(&storage);
}

#endif

}  // namespace Dynarmic
