#include "PlantAI.h"

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
