#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

in vec4 FragPosLightSpace;

// In an ideal world where we have access to a mesh loader we would be 
// able to use the Albedo and Specular maps created by software like 
// Substance (or InstaLOD?), we could even render to a texture to pre-
// compute this in this app. As we are dealing only with geometric 
// primitives I am just leaving a sampler here for getting textures 
// just in case, but I would probably hard-code the Albedo and Specular
// values due to the time factor.
//uniform sampler2D texture1;
// Get the shadow map.
uniform sampler2D shadowMap;

// This corresponds to the camera.
uniform vec3 lightPos;
uniform float time;

float ShadowBias( float d )
{

	return max( 0.05 * ( 1.0 - d ), 0.005 );

}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
	float d = dot(normal, lightDir);

	float shadow = 0.0;

	float bias = ShadowBias( d );
	// check whether current frag pos is in shadow
	// float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
	// PCF
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
    
	// keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if(projCoords.z > 1.0)
		shadow = 0.0;
        
    return shadow;
}

void main()
{    
    // Send the fragment's position.
    gPosition = FragPos;
    // And the normals (per fragment).
    gNormal.xyz = normalize( Normal.xyz );
	gNormal.w = 1.0 - ShadowCalculation( FragPosLightSpace );
    // Ideal world this would be something better, but if we have proper
	// normals for our geometry we would be able to get something good 
	// enough from a colour defined here.
	// Simulate some basic lighting to keep the scene from being too boring.
	// Uncomment to play with this idea, it may work?
	/*vec3 lig = normalize( vec3( 10.0, 4.0, 2.0 ) - FragPos ); // Ligth source.
	vec3 rayDirection = normalize( vec3( 0 ) - lig );
	float dif = max( dot( lig, gNormal.xyz ), 0.0 );

	vec3 diff = dif * vec3( 0.3, 0.25, 0.22 );

	vec3 ref = reflect( lig, gNormal.xyz );
	float spe = pow( max( dot( rayDirection, ref ), 0.0 ), 64.0 );

	gAlbedoSpec.rgb = diff;
	gAlbedoSpec.a = spe;*/

	// Otherwise simply define a colour of your liking and be done!
    gAlbedoSpec.rgb = vec3( 0.5 );//vec3( Normal * 0.5 + 0.5 );
    // We are using the alpha channel of the gAlbedoSpec vec3 to store 
	// our specular world, again in an ideal world this would be a 
	// pre-computed specular map. 
    gAlbedoSpec.a = 1.0;
}