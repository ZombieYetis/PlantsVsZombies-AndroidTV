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

#ifndef HOMURA_MEMORYUTILS_H
#define HOMURA_MEMORYUTILS_H

#include <sys/mman.h>

#include <cstdint>

#include <span>
#include <string>
#include <vector>

namespace homura {

/**
 * @brief 修改从地址 address 开始, 长度为 length 字节的内存的访问保护.
 *
 * @param [in] address 目标内存地址 (不要求对齐).
 * @param [in] length 目标内存区域大小 (不要求对齐).
 * @param [in] prot 新的内存保护标志.
 * @return 是否成功修改.
 */
bool SetProtection(std::uintptr_t address, std::size_t length, int prot);

[[nodiscard]] bool GetProtection(std::uintptr_t address, int &prot);

[[nodiscard]] bool InSamePage(std::uintptr_t addr1, std::uintptr_t addr2);

bool WriteMemory(std::uintptr_t address, std::span<const std::uint8_t> buffer);

/**
 * @brief 获取动态库的加载地址.
 */
[[nodiscard]] std::uintptr_t GetLibBaseAddr(std::string_view libName);

class Patcher {
public:
    Patcher() = default;

    Patcher(std::string_view libName, std::uintptr_t offset, std::vector<std::uint8_t> patchBytes);

    Patcher(const Patcher &) = delete;
    Patcher &operator=(const Patcher &) = delete;

    Patcher(Patcher &&) = default;
    Patcher &operator=(Patcher &&) = default;

    [[nodiscard]] static Patcher CreateWithStr(std::string_view libName, std::uintptr_t offset, std::string patchBytesStr);

    [[nodiscard]] bool HasModified() const noexcept {
        return hasModified_;
    }

    bool Modify();
    bool Restore();

    bool Switch() {
        return !HasModified() ? Modify() : Restore();
    }

protected:
    std::uintptr_t address_ = 0;
    std::vector<std::uint8_t> patchBytes_;
    std::vector<std::uint8_t> originBytes_;
    bool hasModified_ = false;
};

} // namespace homura

#endif // HOMURA_MEMORYUTILS_H
