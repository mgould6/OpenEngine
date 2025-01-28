// Updated DebugTools.h
#ifndef DEBUGTOOLS_H
#define DEBUGTOOLS_H

#include "../model/Model.h"
#include "../model/Camera.h"
#include <glm/glm.hpp>

namespace DebugTools {
    void renderBoneHierarchy(Model* model, const Camera& camera);
    void renderSkeleton(Model* model, const glm::mat4& view, const glm::mat4& projection);
}

#endif // DEBUGTOOLS_H