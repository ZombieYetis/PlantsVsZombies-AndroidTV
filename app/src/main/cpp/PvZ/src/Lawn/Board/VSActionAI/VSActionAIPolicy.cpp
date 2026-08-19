#include "VSActionAIPolicy.h"

#include "PvZ/Lawn/Widget/VSSetupAddonWidget.h"

namespace vsai::detail {

AIEnhancementPolicy GetAIEnhancementPolicy(VSSide side) {
    return {
        .enabled = vsai::IsEnhancedAIEnabled() && vsai::IsSideEnabled(side),
    };
}

} // namespace vsai::detail
