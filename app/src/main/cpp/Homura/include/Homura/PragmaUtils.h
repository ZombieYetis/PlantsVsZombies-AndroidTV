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

#ifndef HOMURA_PRAGMAUTILS_H
#define HOMURA_PRAGMAUTILS_H

// 'warning' pragma: https://learn.microsoft.com/en-us/cpp/preprocessor/warning
// MSVC warnings: https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-c4800-through-c4999
#if defined(_MSC_VER) && !defined(__clang__)
#define DISABLE_MSVC_WARNING_BEGIN(warningNumber) __pragma(warning(push)) __pragma(warning(disable : warningNumber))
#define DISABLE_MSVC_WARNING_END __pragma(warning(pop))
#else
#define DISABLE_MSVC_WARNING_BEGIN(warningNumber)
#define DISABLE_MSVC_WARNING_END
#endif

#define HOMURA_PRAGMA(x) _Pragma(#x)

// 'diagnostic' pragma: https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
#if defined(__GNUC__) || defined(__clang__)
#define DISABLE_GNU_LIKE_WARNING_BEGIN(diagFlag) _Pragma("GCC diagnostic push") HOMURA_PRAGMA(GCC diagnostic ignored diagFlag)
#define DISABLE_GNU_LIKE_WARNING_END _Pragma("GCC diagnostic pop")
#else
#define DISABLE_GNU_LIKE_WARNING_BEGIN(diagFlag)
#define DISABLE_GNU_LIKE_WARNING_END
#endif

// GCC warnings: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
#if defined(__GNUC__) && !defined(__clang__)
#define DISABLE_GCC_WARNING_BEGIN(diagFlag) DISABLE_GNU_LIKE_WARNING_BEGIN(diagFlag)
#define DISABLE_GCC_WARNING_END DISABLE_GNU_LIKE_WARNING_END
#else
#define DISABLE_GCC_WARNING_BEGIN(diagFlag)
#define DISABLE_GCC_WARNING_END
#endif

// Clang warnings: https://clang.llvm.org/docs/DiagnosticsReference.html
#ifdef __clang__
#define DISABLE_CLANG_WARNING_BEGIN(diagFlag) DISABLE_GNU_LIKE_WARNING_BEGIN(diagFlag)
#define DISABLE_CLANG_WARNING_END DISABLE_GNU_LIKE_WARNING_END
#else
#define DISABLE_CLANG_WARNING_BEGIN(diagFlag)
#define DISABLE_CLANG_WARNING_END
#endif

#endif // HOMURA_PRAGMAUTILS_H
