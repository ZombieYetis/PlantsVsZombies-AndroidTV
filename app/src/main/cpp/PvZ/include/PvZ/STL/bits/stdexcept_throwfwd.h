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

#ifndef PVZ_STL_BITS_STDEXCEPT_THROWFWD_H
#define PVZ_STL_BITS_STDEXCEPT_THROWFWD_H

/**
 * @file bits/stdexcept_throwfwd.h
 * @see <a href="https://gcc.gnu.org/onlinedocs/gcc-16.1.0/libstdc++/api/a00800.html">stdexcept_throwfwd.h File Reference</a>
 */

namespace pvzstl::detail {

// Helpers for exception objects in <stdexcept>
[[noreturn, gnu::cold]] void throw_logic_error(const char *msg);
[[noreturn, gnu::cold]] void throw_domain_error(const char *msg);
[[noreturn, gnu::cold]] void throw_invalid_argument(const char *msg);
[[noreturn, gnu::cold]] void throw_length_error(const char *msg);
[[noreturn, gnu::cold]] void throw_out_of_range(const char *msg);
[[noreturn, gnu::cold, gnu::format(printf, 1, 2)]] void throw_out_of_range_fmt(const char *fmt, ...);
[[noreturn, gnu::cold]] void throw_range_error(const char *msg);
[[noreturn, gnu::cold]] void throw_overflow_error(const char *msg);
[[noreturn, gnu::cold]] void throw_underflow_error(const char *msg);

} // namespace pvzstl::detail

#endif // PVZ_STL_BITS_STDEXCEPT_THROWFWD_H
