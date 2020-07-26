#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

#define STB_IMAGE_IMPLEMENTATION    
#include "stb_image.h"

#include <iostream>

class RenderEngine
{

public:

	void run()
	{

		initWindow();
		initShaders();
		renderLoop();

	}

private:

	GLFWwindow* window;

	const uint16_t WIDTH = 1200, HEIGHT = 800;
	bool framebufferResized = false;

	// Geometry pass.
	Shader* shaderG;
	// Light pass.
	Shader* shaderL;
	// Forward render pass.
	Shader* shaderF;

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

	void renderLoop()
	{

		while( !glfwWindowShouldClose( window ) )
		{

			glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			glfwSwapBuffers( window );
			glfwPollEvents();

		}

		glfwTerminate();

	}

	static void framebufferResizeCallback( GLFWwindow* window, int width, int height )
	{

		auto app = reinterpret_cast<RenderEngine*>( glfwGetWindowUserPointer( window ) );
		app->framebufferResized = true;

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