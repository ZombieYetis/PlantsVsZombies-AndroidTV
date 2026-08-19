/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include "PvZ/Lawn/VSActionAIDecision.h"

#include "VSActionAIStrategy.h"

namespace vsai {

std::unique_ptr<IVSAgent> CreateBuiltinVSAgent(VSSide side) {
    switch (side) {
        case VSSide::Plants:
            return detail::CreatePlantAI();
        case VSSide::Zombies:
            return detail::CreateZombieAI();
    }
    return nullptr;
}

} // namespace vsai
