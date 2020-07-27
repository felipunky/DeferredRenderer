#version 330 core
layout (location = 0) out vec3 outGPosition;
layout (location = 1) out vec4 outGNormal;
layout (location = 2) out vec4 outGAlbedoSpec;

// Get our geometry buffer's data.
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform sampler2D texture1;

uniform vec3 lightColour;
in vec2 TexCoords;

in mat4 invView;
in mat4 invModel;
in mat4 invProj;

in vec4 clipSpace;

vec4 reconstructPos(float z, vec2 uv_f)
{
    vec4 sPos = vec4(uv_f * 2.0 - 1.0, z, 1.0);
    sPos = invProj * invView * sPos;
    return vec4((sPos.xyz / sPos.w ), 1.0);
}

void main()
{

	/*vec3 ndcPos = clipSpace.xyz / clipSpace.w;
	vec2 texCoord = ndcPos.xy * 0.5 + 0.5;
	float sampleDepth = texture( gPosition, texCoord ).r;

	float sampleNdcZ = sampleDepth * 2.0 - 1.0;

	vec3 ndcSample = vec3( ndcPos.xy, sampleNdcZ );
	vec4 hViewPos = invProj * vec4( ndcSample, 1.0 );
	vec3 viewPosition = hViewPos.xyz / hViewPos.w;

	vec3 WorldPos = ( invView * vec4( viewPosition, 1.0 ) ).xyz;
	vec3 objectPosition = ( invModel * vec4( WorldPos, 1.0 ) ).xyz;

	if (abs(objectPosition.x) > 0.5) discard;    
	else if (abs(objectPosition.y) > 0.5) discard;    
	else if (abs(objectPosition.z) > 0.5) discard; 

	outGAlbedoSpec = texture( texture1, texCoord );*/

	vec3 col = vec3( 0 );

	vec2 screenSpace = clipSpace.xy / clipSpace.w;

	vec2 depthUv = screenSpace * 0.5 + 0.5;
	depthUv += vec2( 0.5 / 1200.0, 0.5 / 800.0 );

	float sceneDepth = texture( gPosition, depthUv ).r;

	vec4 worldPos = reconstructPos( sceneDepth, depthUv );
	vec4 localPos = invModel * worldPos;

	float dist = 0.5 - abs( localPos.y );
	float distO = 0.5 - abs( localPos.x );

	vec2 uv = vec2( localPos.x, localPos.y ) + 0.5;
	vec4 pos = texture( gPosition, uv );
	vec4 nor = texture( gNormal, uv );
	vec4 alb = texture( gAlbedoSpec, uv );
	/*outGPosition = pos.xyz;
	outGNormal = nor;
	outGAlbedoSpec = alb;*/

	if( dist > 0.0 && dist > 0.0 )
	{
	
		discard;

	}

	else
	{
	
		outGAlbedoSpec = vec4( 1, 0, 0, 1 );
	
	}

	//outGAlbedoSpec = texture( gAlbedoSpec, uv );

	//vec4 texNorm = texture( gNormal, TexCoords );

}