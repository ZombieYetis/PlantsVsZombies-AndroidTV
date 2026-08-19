#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_PLANT_AI_H

#include "PlantAIPlanning.h"

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
