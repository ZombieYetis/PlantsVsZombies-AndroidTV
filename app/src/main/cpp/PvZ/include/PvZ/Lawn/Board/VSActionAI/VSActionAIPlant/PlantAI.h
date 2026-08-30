/*
 * Copyright (C) 2023-2026 PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_H

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIPlant/PlantAIPlanning.h"

#include <initializer_list>

namespace vsai::detail {

class PlantAI final : public PlantAIPlanning {
    using TemplateTactic = std::optional<VSAction> (PlantAI::*)(const VSGameState &, int, int);
    struct TemplateTacticStep {
        TemplateTactic tactic = nullptr;
        bool useFirepowerRow = false;
    };

    PlantDecisionResult TryOpeningEconomyPhase(const VSGameState &state);
    std::optional<VSAction> TryImmediateMaintenancePhase(const VSGameState &state);
    PlantDecisionResult TryOpeningOutputPhase(const VSGameState &state, const PlantDecisionContext &context);
    std::optional<VSAction> TryFirstTemplateTactic(const VSGameState &state, const PlantDecisionContext &context, std::initializer_list<TemplateTacticStep> tactics);
    PlantDecisionResult TryTemplatePressurePhase(const VSGameState &state, const PlantDecisionContext &context);
    PlantDecisionResult TryEconomyConversionPhase(const VSGameState &state, const PlantDecisionContext &context);
    PlantDecisionResult TryLaneDefensePhase(const VSGameState &state, const PlantDecisionContext &context);
    PlantDecisionResult TryFallbackPhase(const VSGameState &state, const PlantDecisionContext &context);

public:
    std::optional<VSAction> Decide(const VSGameState &state) override;
};

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_H
