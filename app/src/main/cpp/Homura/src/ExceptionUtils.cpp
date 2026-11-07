/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "Homura/ExceptionUtils.h"
#include "Homura/Logger.h"

#include <dlfcn.h>
#include <ucontext.h>
#include <unwind.h>
#include <cinttypes>
#include <csignal>

#include <exception>

[[noreturn]] static void ExceptionHandler() {
    try {
        if (auto eptr = std::current_exception()) {
            std::rethrow_exception(eptr);
        } else {
            LOG_FATAL("Unhandled 'std::terminate()' call");
        }
    } catch (const std::exception &e) {
        LOG_FATAL("Unhandled exception: {}", e.what());
    } catch (...) {
        LOG_FATAL("Unknown exception caught");
    }
    std::abort();
}

void homura::RegisterExceptionHandler() noexcept {
    std::set_terminate(&ExceptionHandler);
}

namespace {
// Keep crash diagnostics out of the allocating C++ formatter. Symbol lookup and
// unwinding are best-effort; print the saved fault registers before either one.
void LogCrashAddress(const char *kind, unsigned frame, uintptr_t pc) {
    const uintptr_t address = pc & ~uintptr_t{1}; // ARM Thumb bit is not part of the module offset.
    Dl_info module{};
    if (dladdr(reinterpret_cast<void *>(address), &module) && module.dli_fbase) {
        __android_log_print(ANDROID_LOG_FATAL,
                            "pvztv",
                            "[CRASH] %s #%02u pc=%" PRIxPTR " %s+0x%" PRIxPTR " (%s+0x%" PRIxPTR ")",
                            kind,
                            frame,
                            pc,
                            module.dli_fname ? module.dli_fname : "?",
                            address - reinterpret_cast<uintptr_t>(module.dli_fbase),
                            module.dli_sname ? module.dli_sname : "?",
                            module.dli_saddr ? address - (reinterpret_cast<uintptr_t>(module.dli_saddr) & ~uintptr_t{1}) : 0);
    } else {
        __android_log_print(ANDROID_LOG_FATAL, "pvztv", "[CRASH] %s #%02u pc=%" PRIxPTR " module=unknown", kind, frame, pc);
    }
}

_Unwind_Reason_Code LogCrashFrame(_Unwind_Context *context, void *arg) {
    auto &frame = *static_cast<unsigned *>(arg);
    const auto pc = static_cast<uintptr_t>(_Unwind_GetIP(context));
    if (!pc || frame >= 32)
        return _URC_END_OF_STACK;
    LogCrashAddress("handler-stack", frame++, pc);
    return _URC_NO_REASON;
}
} // namespace

[[noreturn]] static void AccessViolationHandler(int sig, siginfo_t *info, void *uct) {
    static volatile sig_atomic_t handlingCrash = 0;
    if (handlingCrash)
        std::abort();
    handlingCrash = 1;
    __android_log_print(ANDROID_LOG_FATAL, "pvztv", "[CRASH] base_version=%d signal=%d code=%d fault_address=%p", PVZ_VERSION, sig, info ? info->si_code : 0, info ? info->si_addr : nullptr);
#if defined(__arm__)
    if (uct) {
        const auto &regs = static_cast<ucontext_t *>(uct)->uc_mcontext;
        __android_log_print(ANDROID_LOG_FATAL, "pvztv", "[CRASH] pc=%08lx lr=%08lx sp=%08lx fp=%08lx cpsr=%08lx", regs.arm_pc, regs.arm_lr, regs.arm_sp, regs.arm_fp, regs.arm_cpsr);
        __android_log_print(ANDROID_LOG_FATAL,
                            "pvztv",
                            "[CRASH] r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx r6=%08lx",
                            regs.arm_r0,
                            regs.arm_r1,
                            regs.arm_r2,
                            regs.arm_r3,
                            regs.arm_r4,
                            regs.arm_r5,
                            regs.arm_r6);
        __android_log_print(ANDROID_LOG_FATAL, "pvztv", "[CRASH] r7=%08lx r8=%08lx r9=%08lx r10=%08lx ip=%08lx", regs.arm_r7, regs.arm_r8, regs.arm_r9, regs.arm_r10, regs.arm_ip);
        LogCrashAddress("fault-PC", 0, regs.arm_pc);
        LogCrashAddress("saved-LR", 0, regs.arm_lr);
    }
#else
    (void)uct;
#endif
    // This starts in the signal handler, not at the fault PC. Some unwinders
    // cannot cross the signal frame; the saved PC/LR above remain authoritative.
    unsigned frame = 0;
    _Unwind_Backtrace(LogCrashFrame, &frame);
    __android_log_print(ANDROID_LOG_FATAL, "pvztv", "[CRASH] handler-stack end frames=%u (limit=32)", frame);
    std::abort();
}

void homura::RegisterAccessViolationHandler() {
    struct sigaction sa{
        .sa_sigaction = &AccessViolationHandler,
        .sa_flags = SA_SIGINFO,
    };
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
        LOG_ERROR("Failed to register an exception handler");
    }
}
