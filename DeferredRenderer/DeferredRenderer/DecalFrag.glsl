#version 330 compatibility

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gColor;


uniform sampler2D depthMap;
uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform mat4 matrix;

uniform vec3 tr;
uniform vec3 bl;
in vec2 TexCoords;
in vec4 positionSS; // screen space
in vec4 positionWS; // world space

in mat4 invd_mat; // inverse decal matrix

in mat4 matPrjInv; // inverse projection matrix

void clip(vec3 v) {
	if (v.x > tr.x || v.x < bl.x) { discard; }
	if (v.y > tr.y || v.y < bl.y) { discard; }
	if (v.z > tr.z || v.z < bl.z) { discard; }
}

vec2 postProjToScreen(vec4 position)
{
	vec2 screenPos = position.xy / position.w;
	return 0.5 * (vec2(screenPos.x, screenPos.y) + 1);
}
void main() {

	// Calculate UVs
	vec2 UV = postProjToScreen(positionSS);

	// sample the Depth from the Depthsampler
	float Depth = texture2D(depthMap, UV).x * 2.0 - 1.0;

	// Calculate Worldposition by recreating it out of the coordinates and depth-sample
	vec4 ScreenPosition;
	ScreenPosition.xy = UV * 2.0 - 1.0;
	ScreenPosition.z = (Depth);
	ScreenPosition.w = 1.0f;

	// Transform position from screen space to world space
	vec4 WorldPosition = matPrjInv * ScreenPosition;
	WorldPosition.xyz /= WorldPosition.w;
	WorldPosition.w = 1.0f;

	// transform to decal original position and size.
	// 1 x 1 x 1
	WorldPosition = invd_mat * WorldPosition;
	clip(WorldPosition.xyz);

	// Get UV for textures;
	WorldPosition.xy += 0.5;
	WorldPosition.y *= -1.0;

	vec4 bump = texture2D(normalMap, WorldPosition.xy);
	gColor = texture2D(colorMap, WorldPosition.xy);

	//Going to have to do decals in 2 passes..
	//Blend doesn't work with GBUFFER.
	//Lots more to sort out.
	gNormal.xyz = bump;
	gPosition = positionWS;

}