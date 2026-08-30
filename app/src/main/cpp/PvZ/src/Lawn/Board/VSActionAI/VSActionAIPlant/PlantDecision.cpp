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

#include "PvZ/Lawn/Board/VSActionAI/VSActionAIPlant/PlantAI.h"

#include <optional>

namespace vsai::detail {

std::optional<VSAction> PlantAI::Decide(const VSGameState &state) {
    AdvanceBlockedSlots();
    // Priority is intentional: immediate state preservation, replay-aware
    // pressure, economy conversion, and lane defense must be considered in
    // this order against one immutable board snapshot.
    if (const PlantDecisionResult opening = TryOpeningEconomyPhase(state); opening.handled) {
        return opening.action;
    }
    if (std::optional<VSAction> action = TryImmediateMaintenancePhase(state)) {
        return action;
    }

    const PlantDecisionContext context = BuildDecisionContext(state);
    const PlantDecisionResult emergency = TryEmergencyPolicy(state, context);
    if (emergency.handled) {
        return emergency.action;
    }
    if (const PlantDecisionResult phase = TryOpeningOutputPhase(state, context); phase.handled) {
        return phase.action;
    }
    if (const PlantDecisionResult phase = TryTemplatePressurePhase(state, context); phase.handled) {
        return phase.action;
    }
    if (const PlantDecisionResult phase = TryEconomyConversionPhase(state, context); phase.handled) {
        return phase.action;
    }
    if (const PlantDecisionResult phase = TryLaneDefensePhase(state, context); phase.handled) {
        return phase.action;
    }
    return TryFallbackPhase(state, context).action;
}

} // namespace vsai::detail
