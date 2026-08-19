#ifndef PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H
#define PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H

#include "VSActionAIThreat.h"

namespace vsai::detail {

struct ZombieLanePolicy {
    bool deploymentBlocked = true;
    bool hasLiveTarget = false;
    bool mowerless = false;
    bool strongMowerlessPlantLane = false;
    bool conversionRoute = false;
    bool allowsAttack = false;
    bool allowsEconomy = false;
};

ZombieLanePolicy EvaluateZombieLanePolicy(const VSGameState &state, int row);
int MowerlessLaneAttackScoreBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount);
int MowerlessLaneDistributionAdjustment(const ZombieLanePolicy &policy, int zombieCount);
int MowerlessLaneCommitmentBonus(const VSGameState &state, const ZombieLanePolicy &policy, int row, int zombieCount);

} // namespace vsai::detail

#endif // PVZ_LAWN_BOARD_VS_ACTION_AI_LANE_POLICY_H
