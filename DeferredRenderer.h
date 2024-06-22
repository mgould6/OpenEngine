#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <glad/glad.h>

class DeferredRenderer {
public:
    DeferredRenderer();
    void init(unsigned int width, unsigned int height);
    void render();

private:
    unsigned int gBuffer;
    unsigned int gPosition, gNormal, gAlbedoSpec;
};

#endif // DEFERRED_RENDERER_H
