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

#include "PvZ/STL/bits/stdexcept_throw.h"

#include <cstdarg>

#include <memory>

// Private helper to throw logic error if snprintf_lite runs out
// of space (which is not expected to ever happen).
// NUL-terminates buf.
[[noreturn]] static void throw_insufficient_space(const char *buf, const char *bufend) {
    // Include space for trailing NUL.
    const std::size_t len = bufend - buf + 1;

    const char err[] = "not enough space for format expansion "
                       "(Please submit full bug report at https://gcc.gnu.org/bugs/):\n    ";
    const std::size_t errlen = sizeof(err) - 1;

    const auto e = std::make_unique_for_overwrite<char[]>(errlen + len);

    std::memcpy(e.get(), err, errlen);
    std::memcpy(e.get() + errlen, buf, len - 1);
    e[errlen + len - 1] = '\0';
    pvzstl::detail::throw_logic_error(e.get());
}

// Private routine to append decimal representation of VAL to the given
// BUFFER, but not more than BUFSIZE characters.
// Does not NUL-terminate the output buffer.
// Returns number of characters appended, or -1 if BUFSIZE is too small.
static int concat_size_t(char *buf, std::size_t bufsize, std::size_t val) {
    // Long enough for decimal representation.
    int ilen = 3 * sizeof(val);
    auto cs = std::make_unique_for_overwrite<char[]>(ilen);
    char *out = cs.get() + ilen;
    do {
        *--out = "0123456789"[val % 10];
        val /= 10;
    } while (val != 0);
    std::size_t len = cs.get() + ilen - out;
    if (bufsize < len) {
        return -1;
    }

    std::memcpy(buf, cs.get() + ilen - len, len);
    return len;
}

namespace pvzstl_cxx {

// Private routine to print into buf arguments according to format,
// not to exceed bufsize.
// Only '%%', '%s' and '%zu' format specifiers are understood.
// Returns number of characters printed (excluding terminating NUL).
// Always NUL-terminates buf.
// Throws logic_error on insufficient space.
int snprintf_lite(char *buf, std::size_t bufsize, const char *fmt, std::va_list ap) {
    char *d = buf;
    const char *s = fmt;
    const char *const limit = d + bufsize - 1; // Leave space for NUL.

    while (s[0] != '\0' && d < limit) {
        if (s[0] == '%') {
            switch (s[1]) {
                default: // Stray '%'. Just print it.
                    break;
                case '%': // '%%'
                    s += 1;
                    break;
                case 's': // '%s'.
                {
                    const char *v = va_arg(ap, const char *);

                    while (v[0] != '\0' && d < limit) {
                        *d++ = *v++;
                    }

                    if (v[0] != '\0') {
                        // Not enough space for fmt expansion.
                        throw_insufficient_space(buf, limit);
                    }

                    s += 2; // Step over %s.
                    continue;
                } break;
                case 'z':
                    if (s[2] == 'u') // '%zu' -- expand next size_t arg.
                    {
                        const int len = concat_size_t(d, limit - d, va_arg(ap, std::size_t));
                        if (len > 0) {
                            d += len;
                        } else {
                            // Not enough space for fmt expansion.
                            throw_insufficient_space(buf, limit);
                        }

                        s += 3; // Step over %zu
                        continue;
                    }
                    // Stray '%zX'. Just print it.
                    break;
            }
        }
        *d++ = *s++;
    }

    if (s[0] != '\0') {
        // Not enough space for fmt expansion.
        throw_insufficient_space(buf, limit);
    }

    *d = '\0';
    return d - buf;
}

} // namespace pvzstl_cxx
