#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

// In an ideal world where we have access to a mesh loader we would be 
// able to use the Albedo and Specular maps created by software like 
// Substance (or InstaLOD?), we could even render to a texture to pre-
// compute this in this app. As we are dealing only with geometric 
// primitives I am just leaving a sampler here for getting textures 
// just in case, but I would probably hard-code the Albedo and Specular
// values due to the time factor.
//uniform sampler2D texture1;

// This corresponds to the camera.
uniform vec3 rayPos;
uniform vec3 rayDirection;

void main()
{    
    // Send the fragment's position.
    gPosition = FragPos;
    // And the normals (per fragment).
    gNormal = normalize(Normal);
    // Ideal world this would be something better, but if we have proper
	// normals for our geometry we would be able to get something good 
	// enough from a colour defined here.
	vec3 lig = normalize( vec3( 1.0, 0.8, 0.6 ) - FragPos ); // Ligth source.
	float dif = max( dot( lig, gNormal ), 0.0 );

	vec3 diff = dif * vec3( 1.0 );

	vec3 ref = reflect( lig, gNormal );
	float spe = pow( max( dot( rayDirection, ref ), 0.0 ), 32.0 );

    gAlbedoSpec.rgb = vec3( 0.5 );//vec3( Normal * 0.5 + 0.5 );
    // We are using the alpha channel of the gAlbedoSpec vec3 to store 
	// our specular world, again in an ideal world this would be a 
	// pre-computed specular map. 
    gAlbedoSpec.a = 10.0;
}