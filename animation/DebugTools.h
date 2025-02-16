// Updated DebugTools.h
#ifndef DEBUGTOOLS_H
#define DEBUGTOOLS_H

#include "../model/Model.h"
#include "../model/Camera.h"
#include <glm/glm.hpp>

namespace DebugTools {
    void renderBoneHierarchy(Model* model, const Camera& camera);
    void renderSkeleton(Model* model, const glm::mat4& view, const glm::mat4& projection);
    // Logs the decomposed transform (translation and scale) for debugging.
    void logDecomposedTransform(const std::string& boneName, const glm::mat4& transform);

}

#endif // DEBUGTOOLS_H