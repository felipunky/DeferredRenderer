#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
// We need to send the depth map in light space to our fragment shader.
out vec4 FragPosLightSpace;

uniform mat4 lightSpaceMatrix;

void main()
{

    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);

}