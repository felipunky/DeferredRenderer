#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"

#include <vector>
#include <chrono>
#include <iostream>
#include <random>

// This engine is heavily based on:
// https://learnopengl.com/Advanced-Lighting/Deferred-Shading
// https://research.ncl.ac.uk/game/mastersdegree/graphicsforgames/deferredrendering/Tutorial%2015%20-%20Deferred%20Rendering.pdf
// and coding concepts from https://vulkan-tutorial.com/

class RenderEngine
{

public:

	void run()
	{

		initWindow();
		initShaders();
		setupGBuffer();
		initGeometry();
		renderLoop();

	}

private:

	GLFWwindow* window;

	// Original size of the window.
	const uint16_t WIDTH = 1200, HEIGHT = 800;
	bool framebufferResized = false;

	// Shader pointers.
	// Geometry pass.
	Shader* shaderG;
	// Light pass.
	Shader* shaderL;
	// Forward render pass.
	Shader* shaderF;

	// GBuffer ids.
	unsigned int gBuffer;
	unsigned int gPosition, gNormal, gAlbedoSpec;

	// User interaction.
	// OpenGL is right handed so the last component corresponds to move forward/backwards.
	glm::vec3 camPos = glm::vec3( 0.0f, 2.0f, 3.0f ); 
	glm::vec3 camFront = glm::vec3( 0.0f, 0.0f, -1.0f );
	glm::vec3 camUp = glm::vec3( 0.0f, 1.0f, 0.0f );
	// Mouse rotation globals.
	bool firstMouse = true, clicked = false;
	float lastX = ( float )( WIDTH ) / 2.0F, lastY = ( float )( HEIGHT ), yaw = -90.0f, pitch = 0.0f;

	// How much time between frames.
	float deltaTime = 0.0f, lastFrame = 0.0f;

	// Geometry's ids.
	GLuint quadVertexArrayObject, quadVertexBufferObject, quadElementBufferObject,
		   screenQuadVertexArrayObject, screenQuadVertexBufferObject;

	// Lights.
	const uint16_t NUMBER_OF_LIGHTS = 3;
	const float constant = 1.0;
	const float linear = 0.7;
	const float quadratic = 1.8;
	std::vector<glm::vec3> lightPositions, lightColours;

	void initWindow()
	{

		// I am using GLFW to abstract the OS stuff for windows and interaction.
		glfwInit();
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
		glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

		window = glfwCreateWindow( WIDTH, HEIGHT, "Render Engine", nullptr, nullptr );
		if( window == NULL )
		{
		
			throw std::runtime_error( "Failed to create a window!" );
		
		}

		glfwSetWindowUserPointer( window, this );
		glfwMakeContextCurrent( window );
		glfwSetFramebufferSizeCallback( window, framebufferResizeCallback );
		glfwSetMouseButtonCallback( window, mouseClickCallBack );
		glfwSetCursorPosCallback( window, mouseCallback );
		glfwSetWindowAspectRatio( window, HEIGHT, WIDTH );

		glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );

		// glad is used to get function pointer to OpenGL at runtime.
		if( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) )
		{
		
			throw std::runtime_error( "Unable to initialize glad!" );
		
		}

		glEnable( GL_DEPTH_TEST );

	}

	void initShaders()
	{

		// Although there is no consensus on the extension for shader files, I am using a plugin for VS2019
		// that enables some syntax highlighting and auto-completion called GLSL Language Integration and it
		// accepts this two: "*.vert" and "*.frag".
		shaderG = &Shader( "gBuffer.vert", "gBuffer.frag" ); // Watchout for the order! It's not a compiler bug!
		shaderL = &Shader( "lightBuffer.vert", "lightBuffer.frag" );
		shaderF = &Shader( "forward.vert", "forward.frag" );

	}

	void setupGBuffer()
	{

		glGenFramebuffers(1, &gBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		// position color buffer
		glGenTextures(1, &gPosition);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
		// normal color buffer
		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
		// color + specular color buffer
		glGenTextures(1, &gAlbedoSpec);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
		// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
		unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, attachments);
		// create and attach depth buffer (renderbuffer)
		unsigned int rboDepth;
		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
		// finally check if framebuffer is complete
		if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		{

			throw std::runtime_error( "Framebuffer not complete!" );
		
		}
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Configure the gBuffer textures.
		shaderL->use();
		shaderL->setInt( "gPosition", 0 );
		shaderL->setInt( "gNormal", 1 );
		shaderL->setInt( "gAlbedoSpec", 2 );

	}

	void renderLoop()
	{

		// No need to compute this every frame as the FOV stays always the same.
		glm::mat4 projection = glm::perspective( glm::radians( 45.0f ), ( float ) WIDTH / ( float ) HEIGHT,
												 0.1f, 100.0f
												);
		// Now send it to the shaders.
		shaderG->use();
		shaderG->setMat4( "projection", projection );

		shaderF->use();
		shaderF->setMat4( "projection", projection );

		while( !glfwWindowShouldClose( window ) )
		{

			// Calculate the time between frames.
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

			deltaTime = time - lastFrame;
			lastFrame = time;

			// User interaction.
			processInput( window );

			glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			
			// Create the camera (eye).
			glm::mat4 view = glm::lookAt( camPos, camPos + camFront, camUp );
			// Create ant initialize the model matrix.
			glm::mat4 model = glm::mat4( 1.0f );

			// 1st pass, this is when the geometry is added into the gBuffer.
			glBindFramebuffer( GL_FRAMEBUFFER, gBuffer );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			// Don't forget to activate, set the shader's index when adding uniforms!
			shaderG->use();
			shaderG->setMat4( "view", view );

			// TODO: find a cool way to distribute geometry without a LUT
			//for( uint16_t i = 0; i <  )
			model = glm::scale( model, glm::vec3( 3.0 ) );
			model = glm::rotate( model, glm::radians( -90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
			shaderG->setMat4( "model", model );
			quad( &quadVertexArrayObject, &quadVertexBufferObject, &quadElementBufferObject );

			glBindFramebuffer( GL_FRAMEBUFFER, 0 ); // Unbind.

			// 2nd pass, this is when the lighting is calculated by the lightBuffer.frag shader.
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			shaderL->use();
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, gPosition );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, gNormal );
			glActiveTexture( GL_TEXTURE2 );
			glBindTexture( GL_TEXTURE_2D, gAlbedoSpec );

			// Send the spotlight.
			shaderL->setVec3( "spotLight.Position", camPos );
			shaderL->setVec3( "spotLight.RayDirection", camFront );
			shaderL->setVec3( "spotLight.Colour", glm::cos( glm::vec3( 1.0f, 0.0f, 0.0f ) ) );
			shaderL->setFloat( "spotLight.Cutoff", glm::cos( glm::radians( 12.5 ) ) );
			shaderL->setFloat( "spotLight.OuterCutoff", glm::radians( 17.5 ) );

			// Send the camera.
			shaderL->setVec3( "viewPos", camPos );

			for( uint16_t i = 0; i < lightPositions.size(); ++i )
			{
			
				lightPositions[i] = glm::vec3( 3.0f * sin( lastFrame + i ), 0.5f, 3.0f * cos(lastFrame + i) );
				shaderL->setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
				shaderL->setVec3("lights[" + std::to_string(i) + "].Color", lightColours[i]);
				// update attenuation parameters and calculate radius
				shaderL->setFloat("lights[" + std::to_string(i) + "].Linear", linear);
				shaderL->setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
				// then calculate radius of light volume/sphere
				const float maxBrightness = std::fmaxf(std::fmaxf(lightColours[i].r, lightColours[i].g), lightColours[i].b);
				float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
				shaderL->setFloat("lights[" + std::to_string(i) + "].Radius", radius);
			
			}

			glfwSwapBuffers( window );
			glfwPollEvents();

		}

		// Don't leak!
		free();
		glfwTerminate();

	}

	void initGeometry()
	{

		for( uint16_t i = 0; i < NUMBER_OF_LIGHTS; ++i )
		{

			lightPositions.push_back( glm::vec3( 0.0f ) );
			lightColours.push_back( glm::vec3( get_random(), get_random(), get_random() ) );

		}

	}

	// https://stackoverflow.com/questions/686353/random-float-number-generation
	float get_random()
	{
		static std::default_random_engine e;
		static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
		return dis(e);
	}

	void quad( GLuint* vertexArrayObject, GLuint* vertexBufferObject, GLuint* elementBufferObject )
	{

		std::vector<GLfloat> vertices =
		{

			// positions          // colors           // texCoords  // normals
			 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   0.0, 0.0, 1.0f, // top right
			 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   0.0, 0.0, 1.0f, // bottom right
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   0.0, 0.0, 1.0f, // bottom left
			-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,   0.0, 0.0, 1.0f  // top left 

			// To compute the normals for a quad we only need to calculate the vector from each point 
			// to each of it's neighbors:
			//                             * A ------------------- * B
			//                   ^         ^                       |
			//                    \        |                       |
			//                     \       |                       |
			//                      \      |                       |
			//                    e  \     |                       |  QUAD
			//                        \    |                       |
			//                         \   |-----                  |
			//                          \  |     |                 |
			//                           \ | D   |                 | C
			//                             *---------------------->* 
			// So if we want to calculate the normal for vertex D, we compute the cross product of vector A - D and
			// vector C - D, which will give us the perpendicular/normal of D = e.
		};

		std::vector<GLuint> indices =
		{

			0, 1, 3, // first triangle
			1, 2, 3  // second triangle

		};

		// Create unique ID's for each of the OpenGL's objects.
		glGenVertexArrays( 1, vertexArrayObject );
		glGenBuffers( 1, vertexBufferObject );
		glGenBuffers( 1, elementBufferObject );

		// Bind the VAO.
		glBindVertexArray( *vertexArrayObject );

		// Bind the VBO. Watchout for the stride as we are calculating it in bytes (specially when using 
		// vectors).
		glBindBuffer( GL_ARRAY_BUFFER, *vertexBufferObject);
		glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( GLfloat ), vertices.data(), GL_STATIC_DRAW );

		// Bind the EBO.
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, *elementBufferObject );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), 
					  GL_STATIC_DRAW );

		// Position attribute.
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof( GLfloat ), ( void* )( 0 ) );
		glEnableVertexAttribArray( 0 );

		// Colour attribute.
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof( GLfloat ), ( void* )
																				 ( 3 * sizeof( GLfloat ) ) );
		glEnableVertexAttribArray( 1 );

		// Texture Coords attribute.
		glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof( GLfloat ), ( void* )
			(											 6 * sizeof( GLfloat ) ) );
		glEnableVertexAttribArray( 2 );

		// Normal attribute.
		glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof( GLfloat ), ( void* )
															( 8 * sizeof( GLfloat ) ) );
		glEnableVertexAttribArray( 3 );

		glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

	}

	void screenQuad( GLuint* quadVAO, GLuint* quadVBO )
	{
		if( *quadVAO == 0 )
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays( 1, quadVAO );
			glGenBuffers( 1, quadVBO );
			glBindVertexArray( *quadVAO );
			glBindBuffer( GL_ARRAY_BUFFER, *quadVBO );
			glBufferData( GL_ARRAY_BUFFER, sizeof( quadVertices ), &quadVertices, GL_STATIC_DRAW );
			glEnableVertexAttribArray( 0 );
			glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), ( void* ) 0 );
			glEnableVertexAttribArray( 1 );
			glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), 
															( void* )( 3 * sizeof( float ) ) );
		}

		glBindVertexArray( *quadVAO );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		glBindVertexArray( 0 );
	}

	void free()
	{

		// Apparently freeing resources is something that the driver must do!
		// https://community.khronos.org/t/vao-deleting-causes-strange-memory-problems/74691
		glDeleteBuffers( 1, &quadVertexBufferObject );
		glDeleteBuffers( 1, &quadElementBufferObject );
		glDeleteVertexArrays( 1, &quadVertexArrayObject );

		glDeleteTextures( 1, &gPosition );
		glDeleteTextures( 1, &gNormal );
		glDeleteTextures( 1, &gAlbedoSpec );
		glDeleteFramebuffers( 1, &gBuffer );

	}

	// Callbacks
	static void framebufferResizeCallback( GLFWwindow* window, int width, int height )
	{

		auto app = reinterpret_cast<RenderEngine*>( glfwGetWindowUserPointer( window ) );
		app->framebufferResized = true;

	}

	void processInput( GLFWwindow* window )
	{

		float camSpeed = deltaTime * 3.0f;

		// Close the app when pressing the ESC key.
		if( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS )
		{
		
			glfwSetWindowShouldClose( window, true );
		
		}

		// To keep everything frame rate independent the "tick" is used.

		// Move forward. Simple vector addition every scalar in camPos added to every scalar in camFront
		// "weighted" by the deltaTime to abstract the frame dependency.
		// Adding two vector gives a sort of average between two vectors.
		//                 * B
		//                 ^^
		//                 | \  camFront
		//         camPos  |  \
		//                 |   ^ 
		//                 |  /
		//                 | /  camPosOld 
		//                 |/
		//                 * A
		if( glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS )
		{

			camPos += camSpeed * camFront;

		}

		// Same as forward but doing the opposite operation. This 
		if( glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS )
		{

			camPos -= camSpeed * camFront;

		}

		//                 ^
		//          camUp  |  ^
		//               --| / camFront
		//               | |/
		//     side  <------
		// The cross product gives a vector orthogonal to to other vectors, so the cross product of camFront
		// and camUp gives us a vector pointing to the "side" of the player, multiplying by deltaTime to make
		// sure we don't rely on the frame rate. If we apply the same knowledge of vector addition and
		// subtraction we are able to use this vector to move in that direction.
		if( glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS )
		{

			camPos -= glm::normalize( glm::cross( camFront, camUp ) ) * camSpeed;

		}

		if( glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS )
		{

			camPos += glm::normalize( glm::cross( camFront, camUp ) ) * camSpeed;

		}

	}

	static void mouseCallback( GLFWwindow* window, double xPos, double yPos )
	{

		// Shameless copy from learnopengl.com
		// TODO:
		// Add quaternion support!
		auto app = reinterpret_cast<RenderEngine*>( glfwGetWindowUserPointer( window ) );

		if( app->firstMouse || !app->clicked )
		{
			app->lastX = xPos;
			app->lastY = yPos;
			app->firstMouse = false;
		}

		float xoffset = xPos - app->lastX;
		float yoffset = app->lastY - yPos; // reversed since y-coordinates go from bottom to top
		app->lastX = xPos;
		app->lastY = yPos;

		float sensitivity = 0.1f; // Who doesn't like magic values?
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		app->yaw += xoffset;
		app->pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (app->pitch > 89.0f)
			app->pitch = 89.0f;
		if (app->pitch < -89.0f)
			app->pitch = -89.0f;

		// Kill me math wizards, or lock me then Gimbal...
		glm::vec3 front;
		front.x = cos( glm::radians( app->yaw ) ) * cos( glm::radians( app->pitch ) );
		front.y = sin( glm::radians( app->pitch ) );
		front.z = sin( glm::radians( app->yaw ) ) * cos( glm::radians( app->pitch ) );
		app->camFront = glm::normalize( front );

	}

	static void mouseClickCallBack( GLFWwindow* window, int button, int action, int mods )
	{
	
		auto app = reinterpret_cast<RenderEngine*>( glfwGetWindowUserPointer( window ) );

		if( button == GLFW_MOUSE_BUTTON_LEFT )
		{
		
			if( action == GLFW_PRESS && !( app->clicked ) )
			{

				app->clicked = true;

			}

			else if( action == GLFW_RELEASE && ( app->clicked ) )
			{

				app->clicked = false;

			}
		
		}
	
	}

};

int main()
{

	RenderEngine engine;

	try
	{

		engine.run();

	}

	catch( const std::exception& e )
	{

		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;

	}

	return EXIT_SUCCESS;

}