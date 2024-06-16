#version 330 core

void main()
{
    // Store the fragment's depth in the depth buffer
    gl_FragDepth = gl_FragCoord.z;
}
