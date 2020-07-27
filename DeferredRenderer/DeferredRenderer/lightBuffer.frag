#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// Get our geometry buffer's data.
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Custom SpotLight structure to keep everything organized.
struct SpotLight
{

	vec3 Position;
	vec3 RayDirection;
	vec3 Colour;
	float Cutoff;
	float OuterCuttoff;

};
uniform SpotLight spotLight;

// Same goes for our point lights.
struct Light 
{

    vec3 Position;
    vec3 Colour;
    
    float Linear;
    float Quadratic;
    float Radius;

};

// Unfortunately we can't send the number of lights as a uniform to get this value 
// automatically, so just hard-code it here for our loop.
const int NR_LIGHTS = 30;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

float decay( vec3 lightPos, vec3 fragPos, float Kl, float Kq )
{

	float dist = distance( lightPos, fragPos );

	return 1.0 / ( 1.0 + Kl * dist + Kq * ( dist * dist ) );

}

float distSquared( vec3 A, vec3 B )
{

	vec3 C = A - B;
	return dot( C, C );

}

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * 1.0 * 7.5) / (7.5 + 1.0 - z * (7.5 - 1.0));	
}

void main()
{             
    // Get the data from the gBuffer.
    vec3 FragPos = texture( gPosition, TexCoords ).rgb;
    vec4 Normal = texture( gNormal, TexCoords );
    vec3 Diffuse = texture( gAlbedoSpec, TexCoords ).rgb;
    float Specular = texture( gAlbedoSpec, TexCoords ).a;
    
	// Make sure that we do not colour not geometric stuff.
	if( Normal.xyz == vec3( 0 ) ) discard;

    // Normal lighting calculations.
    vec3 lighting  = Diffuse * 0.1; // TODO: Find a better way of doing this.
    vec3 viewDir  = normalize( viewPos - FragPos );
    
	for( int i = 0; i < NR_LIGHTS; ++i )
    {
        // Early escape when the distance between the fragment and the light 
		// is smaller than the light volume/sphere threshold.
        //float dist = length(lights[i].Position - FragPos);
        //if(dist < lights[i].Radius)
        // Let's optimize by skipping the expensive square root calculation
		// for when it's necessary.
		float dist = distSquared( lights[i].Position, FragPos );
		if( dist < lights[i].Radius * lights[i].Radius )
		{
            // diffuse
			// http://learnwebgl.brown37.net/09_lights/lights_diffuse.html
			// The dot gives a scalar from two vectors (d) which corresponds to the projection (multiplying B * d) of A into
			// B. This relation between two vectors helps us determine whether or not the light position is pointing towards
			// the normal or if we are behind (d bigger or smaller than 90, we use a max to make sure we are not taking 
			// diffuse lighting away!) and how much light the surface is getting from the diffuse term. 
			//                 ^
			//                /|
			//            A  / |
			//              /  |     B
			//             ---------------->
            vec3 lightDir = normalize( lights[i].Position - FragPos );
            vec3 diffuse = max( dot( Normal.xyz, lightDir ), 0.0 ) * Diffuse * lights[i].Colour;
            // specular
            vec3 halfwayDir = normalize( lightDir + viewDir );  
            float spec = pow( max( dot( Normal.xyz, halfwayDir ), 0.0 ), 16.0 );
            vec3 specular = lights[i].Colour * spec * Specular;
            // attenuation
			// Don't forget to calculate the square root of the dist variable,
			// we have to do so manually as we optimized by skipping it early.
            float attenuation = decay( lights[i].Position, FragPos, lights[i].Linear ,lights[i].Quadratic );
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }    

	// SpotLight.
	/*vec3 lig = normalize( spotLight.Position - FragPos );
	float dif = max( dot( Normal, lig ), 0.0 );
	float atte = decay( lights[0].Linear, lights[0].Quadratic, length( lig ) );
	vec3 ref = reflect( -lig, Normal );

	float the = max( dot( lig, normalize( -spotLight.RayDirection ) ), 0.0 );
	float eps = spotLight.Cutoff - spotLight.OuterCuttoff;
	float inte = clamp( ( the - spotLight.OuterCuttoff ) / eps, 0.0, 1.0 );

	vec3 diff = inte * atte * dif * Diffuse * spotLight.Colour;
	vec3 spe = inte * atte * spotLight.Colour * pow( max( dot( viewDir, ref ), 0.0 ), 32.0 );
	//diff *= inte;// * atte;
	//spe  *= inte;// * atte;

	lighting += diff + spe;*/

	// SpotLight.
	vec3 lig = normalize( spotLight.Position - FragPos );
	float the = dot( lig, normalize( -spotLight.RayDirection ) );

	float diff = max( dot( Normal.xyz, lig ), 0.0 );
	vec3 ref = reflect( -lig, Normal.xyz );
	float atte = decay( spotLight.Position, FragPos, 0.09, 0.032 );
	float e = spotLight.Cutoff - spotLight.OuterCuttoff;
	float inte = clamp( ( the - spotLight.OuterCuttoff ) / e, 0.0, 1.0 );

	//vec3 amb = inte * atte * objectColours[0];// *0.5 + 0.5 * n.y;
	vec3 dif = inte * Diffuse * spotLight.Colour * diff;
	vec3 spe = inte * Specular * spotLight.Colour * pow( max( dot( viewDir, ref ), 0.0 ), 32.0 );

	lighting += dif + spe;
	// Our shadows.
	lighting *= Normal.w;

	FragColor = vec4( lighting, 1 );
	// Cheap and dirty gamma correction.
    //FragColor = vec4( pow( lighting, vec3( 0.4545 ) ), 1.0 );

}