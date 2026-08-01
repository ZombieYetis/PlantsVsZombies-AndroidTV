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

#include "PvZ/STL/bits/c++config.h"
#include "PvZ/STL/bits/stdexcept_throw.h"

#include <cstdarg>

#include <memory>
#include <stdexcept>

namespace pvzstl_cxx {
int snprintf_lite(char *buf, std::size_t bufsize, const char *fmt, std::va_list ap);
}

void pvzstl::detail::throw_logic_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::logic_error(msg));
}

void pvzstl::detail::throw_domain_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::domain_error(msg));
}

void pvzstl::detail::throw_invalid_argument(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::invalid_argument(msg));
}

void pvzstl::detail::throw_length_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::length_error(msg));
}

void pvzstl::detail::throw_out_of_range(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::out_of_range(msg));
}

void pvzstl::detail::throw_out_of_range_fmt(const char *fmt, ...) {
#if __cpp_exceptions
    const std::size_t len = std::strlen(fmt);
    // We expect at most 2 numbers, and 1 short string. The additional
    // 512 bytes should provide more than enough space for expansion.
    const std::size_t alloca_size = len + 512;
    const auto s = std::make_unique_for_overwrite<char[]>(alloca_size);
    std::va_list ap;

    va_start(ap, fmt);
    pvzstl_cxx::snprintf_lite(s.get(), alloca_size, fmt, ap);
    throw std::out_of_range(s.get());
    va_end(ap); // Not reached.
#else
    throw_out_of_range(msg);
#endif
}

void pvzstl::detail::throw_range_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::range_error(msg));
}

void pvzstl::detail::throw_overflow_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::overflow_error(msg));
}

void pvzstl::detail::throw_underflow_error(const char *msg) {
    PVZSTL_THROW_OR_ABORT(std::underflow_error(msg));
}
