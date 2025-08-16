// AnimationBatchSmoother.cpp
#include "Animation.h"
#include "../common_utils/Logger.h"
#include <vector>
#include <string>

void RunBatchSmoothing(const std::vector<Animation*>& animations)
{
    Logger::log("=== Batch Smoothing: Starting ===", Logger::WARNING);

    for (Animation* anim : animations)
    {
        if (!anim || !anim->isLoaded())
        {
            Logger::log("[SKIP] Null or unloaded animation encountered.", Logger::WARNING);
            continue;
        }

        Logger::log("[RUNNING] Smoothing " + anim->getName(), Logger::WARNING);
        anim->suppressPostBakeJitter();
        Logger::log("[DONE] Smoothing " + anim->getName(), Logger::WARNING);
    }

    Logger::log("=== Batch Smoothing: Complete ===", Logger::WARNING);

}
