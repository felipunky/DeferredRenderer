#version 330 core
// Quad
//layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aColour;
//layout (location = 2) in vec2 aTexCoords; 
//layout (location = 3) in vec3 aNormal;
// Cube

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords; 


out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{

	// We want our fragment's positions and our normals to be in
	// world space to perform our lighting pass later on.
    vec4 worldPos = model * vec4( aPos, 1 );
    FragPos = worldPos.xyz; 
    TexCoords = aTexCoords;
    
    mat3 normalMatrix = transpose( inverse( mat3( model ) ) );
    Normal = normalMatrix * aNormal;

    gl_Position = projection * view * worldPos;

}