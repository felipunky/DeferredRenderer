#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoords;

out mat4 invProj;
out mat4 invView;
out mat4 invModel;

out vec4 clipSpace;

void main()
{

	vec4 worldPos = model * vec4( aPos, 1 );
	TexCoords = aTexCoords;
	invProj = inverse( projection );
	invModel = inverse( model );
	invView = inverse( view );
	gl_Position = projection * view * worldPos;
	clipSpace = gl_Position;

}