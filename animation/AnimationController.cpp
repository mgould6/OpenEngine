#define GLM_ENABLE_EXPERIMENTAL
#include "AnimationController.h"
#include "../common_utils/Logger.h"
#include <fstream>
#include <glm/gtx/component_wise.hpp> // for glm::all(glm::equal …) style helpers
#include <glm/gtc/epsilon.hpp>  // epsilonEqual + all/any/not_ helpers
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>          // core
// optional but handy

#include "DebugTools.h"
#include <GLFW/glfw3.h>
#include <iomanip>   // for std::setw
#include <cmath>        // fmodf, fabsf
#include "Animation.h"
#include <cmath>          // std::fabs

// Forward declaration ­
static glm::mat4 removeScale(const glm::mat4& m);

static void dumpBoneDebugTrace(
    const std::string& boneName,
    int frame,
    const Animation* animation,
    Model* model);


AnimationController::AnimationController(Model* model)
    : model(model)
    , currentAnimation(nullptr)
    , animationTime(0.0f)
{
}


/* return true if every element differs by less than eps */
static bool matNearlyEqual(const glm::mat4& A,
    const glm::mat4& B,
    float            eps = 0.001f)
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (std::fabs(A[c][r] - B[c][r]) > eps)
                return false;
    return true;
}

/*--------------------------------------------------------------
    loadAnimation
    – Registers or reloads a clip and (optionally) makes it
      the active animation.
    – If forceReload == true and a clip with the same name
      already exists, the old one is deleted and replaced.
--------------------------------------------------------------*/
bool AnimationController::loadAnimation(const std::string& name,
    const std::string& filePath,
    bool              forceReload)
{
    /* ----------------------------------------------------------
       1.  Handle duplicates / hot-reloads
    ---------------------------------------------------------- */
    auto it = animations.find(name);

    if (it != animations.end() && !forceReload)
        return true;                       // nothing to do

    if (it != animations.end())            // replace old clip
    {
        delete it->second;
        animations.erase(it);
    }

    /* ----------------------------------------------------------
       2.  Load the new clip
    ---------------------------------------------------------- */
    Animation* clip = new Animation(filePath, model);
    if (!clip->isLoaded())
    {
        Logger::log("Failed to load animation: " + filePath,
            Logger::ERROR);
        delete clip;
        return false;
    }
    animations[name] = clip;

    /* ----------------------------------------------------------
       3.  Auto-bind if this is the selected clip
           (or if nothing is currently playing)
    ---------------------------------------------------------- */
    if (currentAnimation == nullptr || currentAnimation->getName() == name)
    {
        currentAnimation = clip;

        // UPDATE: start slightly after 0 to avoid sampling the bind pose
        animationTime = 1e-5f;

        Logger::log("NOW PLAYING: " + name +
            " | keyframes = " +
            std::to_string(clip->getKeyframeCount()) +
            " | length = " +
            std::to_string(clip->getClipDurationSeconds()) + " s",
            Logger::INFO);

        scanForTranslationJumps(clip);

    }

    /* ----------------------------------------------------------
   OPTIONAL: verify frame 0 pose matches the model bind pose
---------------------------------------------------------- */
    if (clip->isLoaded())
    {
        std::map<std::string, glm::mat4> firstPose;
        clip->interpolateKeyframes(1e-6f, firstPose);   // ~frame 0

        for (const auto& name : { "spine", "thigh.L", "upper_arm.L" }) {
            if (firstPose.count(name)) {
                Logger::log("POSE AT t=0 FOR " + std::string(name) + ":\n" + glm::to_string(firstPose[name]), Logger::WARNING);
            }
            else {
                Logger::log("Pose missing for bone: " + std::string(name), Logger::WARNING);
            }

            glm::mat4 bindLocal = model->getLocalBindPose(name);
            Logger::log("BIND POSE LOCAL FOR " + std::string(name) + ":\n" + glm::to_string(bindLocal), Logger::WARNING);
        }

        bool poseMatchesBind = true;

        for (const auto& bone : model->getBones())
        {
            const std::string& name = bone.name;
            glm::mat4 bindLocal = model->getLocalBindPose(name);
            glm::mat4 poseLocal =
                firstPose.count(name) ? firstPose[name] : bindLocal;

            if (!matNearlyEqual(bindLocal, poseLocal, 1e-5f))
            {
                glm::vec3 tBind(bindLocal[3]);
                glm::vec3 tPose(poseLocal[3]);
                glm::vec3 delta = tPose - tBind;

                Logger::log("[MISMATCH] Bone '" + name + "' differs in frame 0", Logger::WARNING);
                Logger::log("  Bind  T: " + glm::to_string(tBind), Logger::WARNING);
                Logger::log("  Frame0 T: " + glm::to_string(tPose), Logger::WARNING);
                Logger::log("  Delta Translation: " + glm::to_string(delta), Logger::WARNING);

                poseMatchesBind = false;
            }

        }

        if (poseMatchesBind)
            Logger::log("[OK] First frame of '" + name +
                "' matches bind pose.", Logger::INFO);
    }



    return true;
}

void scanForTranslationJumps(const Animation* anim, float threshold)

{
    if (!anim) return;

    const auto& keyframes = anim->getKeyframes();
    if (keyframes.size() < 2) return;

    Logger::log("=== Translation Jitter Scan Start ===", Logger::WARNING);

    for (const auto& bone : anim->getKeyframes().front().boneTransforms)
    {
        const std::string& boneName = bone.first;
        for (size_t i = 0; i < keyframes.size() - 1; ++i)
        {
            const glm::mat4& m0 = keyframes[i].boneTransforms.at(boneName);
            const glm::mat4& m1 = keyframes[i + 1].boneTransforms.at(boneName);

            glm::vec3 t0(m0[3]);
            glm::vec3 t1(m1[3]);
            glm::vec3 delta = t1 - t0;

            if (glm::length(delta) > threshold)
            {
                Logger::log("[JUMP] Bone: " + boneName +
                    " | Frame " + std::to_string(i) + " -> " + std::to_string(i + 1) +
                    " | Delta T = " + glm::to_string(delta),
                    Logger::WARNING);
            }
        }
    }

    Logger::log("=== Translation Jitter Scan Complete ===", Logger::WARNING);
}


void AnimationController::setCurrentAnimation(const std::string& name)
{
    auto it = animations.find(name);
    if (it == animations.end())
    {
        Logger::log("ERROR: Animation not found: " + name, Logger::ERROR);
        return;
    }

    Animation* newClip = it->second;

    // Avoid resetting if this is already the active animation
    if (currentAnimation == newClip)
    {
        Logger::log("INFO: Animation [" + name + "] is already playing.", Logger::INFO);
        return;
    }

    // Run bind mismatch check only once per clip
    if (!newClip->bindMismatchChecked())
        newClip->checkBindMismatch(model);

    currentAnimation = newClip;
    animationTime = 0.00001f;  // Ensure we skip t=0 precision issues

    Logger::log("NOW PLAYING: [" + name + "]"
        "  keyframes=" + std::to_string(currentAnimation->getKeyframeCount()) +
        "  duration=" + std::to_string(currentAnimation->getClipDurationSeconds()) + "s",
        Logger::INFO);
}




const glm::mat4& AnimationController::bindGlobalNoScale
(const std::string& bone) const
{
    static const glm::mat4 I(1.0f);
    if (!model || !model->getBindPose())
        return I;
    return model->getBindPose()->getGlobalNoScale(bone);
}



/*------------------------------------------------------------------
  - deltaTime arrives in seconds from the game loop.
  - We convert it to fractional Assimp "ticks":
        deltaTicks = deltaTime * ticksPerSecond;
  - animationTime is stored in ticks because the rest of the
    controller and applyToModel() expect that unit.
------------------------------------------------------------------*/
/*------------------------------------------------------------------
  Advance the animation clock – seconds domain
------------------------------------------------------------------*/

void AnimationController::update(float deltaTime)
{
    if (!currentAnimation)
        return;

    const auto& keyframes = currentAnimation->getKeyframes();
    if (keyframes.empty())
        return;

    // Handle rewind
    if (debugRewind) {
        debugFrame = 0;
        debugRewind = false;
    }

    // Frame stepping setup
    static float timeAccumulator = 0.0f;
    float ticksPerSecond = currentAnimation->getTicksPerSecond();
    if (ticksPerSecond <= 0.0f) {
        Logger::log("WARNING: ticksPerSecond was 0. Defaulting to 60 FPS.", Logger::WARNING);
        ticksPerSecond = 60.0f;
    }
    const float FRAME_TIME = 1.0f / ticksPerSecond;

    // Auto-play with optional loop
    if (debugPlay)
    {
        timeAccumulator += deltaTime;
        while (timeAccumulator >= FRAME_TIME)
        {
            int lastFrameIndex = static_cast<int>(keyframes.size()) - 1;

            if (!loopPlayback && debugFrame >= lastFrameIndex)
            {
                debugPlay = false;
                timeAccumulator = 0.0f;
                break;
            }

            int nextFrame = debugFrame + 1;
            if (!loopPlayback && nextFrame > lastFrameIndex)
            {
                // Don't allow overstep
                debugPlay = false;
                timeAccumulator = 0.0f;
                break;
            }
            debugFrame = loopPlayback ? (debugFrame + 1) % keyframes.size() : nextFrame;
            timeAccumulator -= FRAME_TIME;
        }
    }

    // Manual step
    if (debugStep) {
        if (loopPlayback) {
            debugFrame = (debugFrame + 1) % static_cast<int>(keyframes.size());
        }
        else {
            if (debugFrame < static_cast<int>(keyframes.size()) - 1)
                debugFrame++;
        }
        debugStep = false;
    }

    debugFrame = std::clamp(debugFrame, 0, static_cast<int>(keyframes.size()) - 1);
    animationTime = keyframes[debugFrame].time;

    Logger::log("DEBUG: Frame #" + std::to_string(debugFrame) +
        " at t=" + std::to_string(animationTime), Logger::INFO);
}



void AnimationController::applyToModel(Model* model)
{
    if (!model || !currentAnimation) return;

    if (debugFrame == 28)
    {
        Logger::log("DEBUG: Frame 31 | animationTime = " + std::to_string(animationTime), Logger::WARNING);
    }

    // 1. local-pose interpolation
    std::map<std::string, glm::mat4> localBoneMatrices;
    if (lockToExactFrame && currentAnimation && debugFrame >= 0 && debugFrame < static_cast<int>(currentAnimation->getKeyframes().size())) {
        const Keyframe& kf = currentAnimation->getKeyframes()[debugFrame];
        for (const auto& [boneName, mat] : kf.boneTransforms) {
            localBoneMatrices[boneName] = mat;
        }
    }
    else {
        currentAnimation->interpolateKeyframes(animationTime, localBoneMatrices);
    }

    for (const auto& bone : model->getBones())
        if (!localBoneMatrices.count(bone.name))
            localBoneMatrices[bone.name] = model->getLocalBindPose(bone.name);

    // 2. build global transforms
    std::map<std::string, glm::mat4> globalBoneMatrices;
    for (const auto& [boneName, _] : localBoneMatrices)
        globalBoneMatrices[boneName] =
        buildGlobalTransform(boneName, localBoneMatrices, model, globalBoneMatrices);

    // 3. final skin matrices + debug logic
    static std::unordered_set<int> dumpedFrames;
    static bool dumpedFrame28 = false;

    bool shouldDump = false;
    int targetFrames[] = { 29, 30, 31 };

    for (int tf : targetFrames) {
        if (debugFrame == tf && !dumpedFrames.count(tf)) {
            dumpedFrames.insert(tf);
            shouldDump = true;
            Logger::log("==== DEBUG DUMP FOR FRAME " + std::to_string(debugFrame) + " ====", Logger::WARNING);
            dumpBoneDebugTrace("DEF-thigh.R", debugFrame, currentAnimation, model);
            dumpBoneDebugTrace("DEF-pelvis", debugFrame, currentAnimation, model);

            break;
        }
    }

    for (const auto& [boneName, globalScaled] : globalBoneMatrices)
    {
        glm::mat4 offsetMatrix = model->getBoneOffsetMatrix(boneName);
        glm::mat4 final = model->getGlobalInverseTransform() * globalScaled * offsetMatrix;

        if (boneName == "thigh.R" && debugFrame >= 0)
        {
            glm::vec3 pos = glm::vec3(final[3]);
            Logger::log("DEBUG: Frame " + std::to_string(debugFrame) +
                " | thigh.R Final Skin Pos: " + glm::to_string(pos),
                Logger::WARNING);
        }

        if (shouldDump)
        {
            glm::mat4 noScale = removeScale(globalScaled);
            Logger::log("Bone: " + boneName, Logger::WARNING);
            Logger::log("  Global With Scale:\n" + glm::to_string(globalScaled), Logger::WARNING);
            Logger::log("  Global No Scale:\n" + glm::to_string(noScale), Logger::WARNING);
            Logger::log("  Final Skin Matrix:\n" + glm::to_string(final), Logger::WARNING);
        }

        model->setBoneTransform(boneName, final);
    }

    // Exit after frame 31 is dumped once
    //avoids relying purely on debugFrame and guarantees that it exits only once per frame dump based on actual animation time, even if debugFrame lingers

    static float lastDumpedTime = -1.0f;
    float currentTime = animationTime;

    //if (debugFrame == 31 && shouldDump && std::abs(currentTime - lastDumpedTime) > 1e-4f) {
    //    Logger::log("=== Frame 31 logged. Exiting for clean log capture. ===", Logger::INFO);
    //    lastDumpedTime = currentTime;
    //    std::exit(0);
    //}


}




glm::mat4 AnimationController::buildGlobalTransform(
    const std::string& boneName,
    const std::map<std::string, glm::mat4>& localBoneMatrices,
    Model* model,
    std::map<std::string, glm::mat4>& globalBoneMatrices
)
{
    // Already computed?
    if (globalBoneMatrices.find(boneName) != globalBoneMatrices.end())
        return globalBoneMatrices[boneName];

    // Get parent's global transform
    glm::mat4 parentGlobal = glm::mat4(1.0f);
    std::string parentName = model->getBoneParent(boneName);
    if (!parentName.empty())
        parentGlobal = buildGlobalTransform(parentName, localBoneMatrices, model, globalBoneMatrices);

    // Get animated local transform if available
    glm::mat4 animatedLocal = glm::mat4(1.0f);
    auto it = localBoneMatrices.find(boneName);
    if (it != localBoneMatrices.end())
        animatedLocal = it->second;
    else
        animatedLocal = model->getLocalBindPose(boneName);  // fallback to bind pose local if no animation!

    // Build global transform
    glm::mat4 globalTransform = parentGlobal * animatedLocal;

    // Cache
    globalBoneMatrices[boneName] = globalTransform;
    return globalTransform;
}




bool AnimationController::isAnimationPlaying() const
{
    return currentAnimation != nullptr;
}

void AnimationController::stopAnimation()
{
    currentAnimation = nullptr;
    animationTime = 0.0f;
    Logger::log("Animation stopped.", Logger::INFO);
}

void AnimationController::resetAnimation()
{
    animationTime = 0.0f;
}


// Remove scale from a TRS matrix but keep translation & rotation
static glm::mat4 removeScale(const glm::mat4& m)
{
    // 1. extract translation (x,y,z from the 4th column)
    glm::vec3 t(m[3].x, m[3].y, m[3].z);

    // 2. extract pure rotation (quat kills scale/shear)
    glm::quat r = glm::quat_cast(m);
    glm::mat4 out = glm::mat4_cast(r);

    // 3. re-attach translation
    out[3] = glm::vec4(t, 1.0f);
    return out;
}


static void dumpBoneDebugTrace(
    const std::string& boneName,
    int frame,
    const Animation* animation,
    Model* model)
{
    if (!animation || !model) return;

    const auto& keyframes = animation->getKeyframes();
    if (frame < 0 || frame >= static_cast<int>(keyframes.size())) return;

    const auto& kf = keyframes[frame];
    glm::mat4 localMatrix;
    auto it = kf.boneTransforms.find(boneName);
    if (it != kf.boneTransforms.end())
        localMatrix = it->second;
    else
        localMatrix = model->getLocalBindPose(boneName);

    // Build full local matrix map
    std::map<std::string, glm::mat4> localBoneMatrices;
    for (const auto& [name, mat] : kf.boneTransforms)
        localBoneMatrices[name] = mat;

    for (const auto& bone : model->getBones())
        if (!localBoneMatrices.count(bone.name))
            localBoneMatrices[bone.name] = model->getLocalBindPose(bone.name);

    // Build global matrices
    std::map<std::string, glm::mat4> globalBoneMatrices;
    glm::mat4 global = localMatrix;
    glm::mat4 parentGlobal = glm::mat4(1.0f);

    std::string parentName = model->getBoneParent(boneName);
    if (!parentName.empty())
        parentGlobal = AnimationController::buildGlobalTransform(parentName, localBoneMatrices, model, globalBoneMatrices);

    global = parentGlobal * localMatrix;

    glm::mat4 offset = model->getBoneOffsetMatrix(boneName);
    glm::mat4 globalInverse = model->getGlobalInverseTransform();
    glm::mat4 skinMatrix = globalInverse * global * offset;

    // Output everything
    Logger::log("==== DEBUG TRACE: " + boneName + " | Frame " + std::to_string(frame) + " ====", Logger::WARNING);
    Logger::log("Local Matrix:\n" + glm::to_string(localMatrix), Logger::WARNING);
    Logger::log("Parent Global Matrix (" + parentName + "):\n" + glm::to_string(parentGlobal), Logger::WARNING);
    Logger::log("Global Matrix:\n" + glm::to_string(global), Logger::WARNING);
    Logger::log("Final Skin Matrix:\n" + glm::to_string(skinMatrix), Logger::WARNING);
}
