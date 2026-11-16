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

#ifndef HOMURA_DYNAMICLIBUTILS_H
#define HOMURA_DYNAMICLIBUTILS_H

namespace homura {

class SharedObjLoader {
public:
    SharedObjLoader(const SharedObjLoader &) = delete;

    SharedObjLoader(SharedObjLoader &&other) noexcept
        : handle_{other.handle_} {
        other.handle_ = nullptr;
    }

    explicit SharedObjLoader([[gnu::nonnull]] const char *filename);
    ~SharedObjLoader();

    SharedObjLoader &operator=(const SharedObjLoader &) = delete;

    SharedObjLoader &operator=(SharedObjLoader &&other) noexcept;

    [[nodiscard]] bool IsOpen() const noexcept {
        return handle_ != nullptr;
    }

    template <typename T = void>
    [[nodiscard]] T *GetSymbol([[gnu::nonnull]] const char *name) const {
        return reinterpret_cast<T *>(GetSymbolImpl(name));
    }

protected:
    [[nodiscard]] void *GetSymbolImpl(const char *name) const;

    void *handle_ = nullptr;
};

} // namespace homura

#endif // HOMURA_DYNAMICLIBUTILS_H
