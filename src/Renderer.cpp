#include "Renderer.hpp"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

Renderer::Renderer()
{
	// --- Shaders ---
	// Vertex Shader: Determines the position of our vertices on screen
	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	// Fragment Shader: Determines the color of the pixels
	const char* fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f); // Orange color\n"
		"}\n\0";

 
	//Build and Compile our Shader Program
	// Compile Vertex Shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	// Compile Fragment Shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Link shaders into a Shader Program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Delete the individual shaders as they're now linked into our program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);



	// Define Vertex Data (The 3 corners of the triangle)
	float vertices[] = {
		-0.5f, -0.5f, 0.0f, // Left  
		 0.5f, -0.5f, 0.0f, // Right 
		 0.0f,  0.5f, 0.0f  // Top   
	};



	// 6. Set up Buffers (VBO) and Arrays (VAO)
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Bind the Vertex Array Object first, then bind and set vertex buffers

	glBindVertexArray(VAO);

	// Copy our vertices array into a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Tell OpenGL how it should interpret the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the VBO and VAO safely
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Initialize FBO to a default size
	Resize(800, 600);
}

Renderer::~Renderer()
{
	// 8. Clean up resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);
}

void Renderer::Render(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// BIND THE FBO (Intercept the draw calls)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// SET VIEWPORT TO MATCH FBO TEXTURE SIZE
	glViewport(0, 0, viewportWidth, viewportHeight);


	// Rendering commands here
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Dark teal background
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw the triangle
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO); // Bind the VAO containing our triangle configuration
	glDrawArrays(GL_TRIANGLES, 0, 3); // 3 represents the number of vertices to draw

	// UNBIND THE FBO (Return to drawing to the screen)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Creates framebuffer and texture from scratch and makes makes them match the new width and height
void Renderer::Resize(int width, int height)
{
	// Prevent resizing to 0 (causes OpenGL crashes)
	if (width <= 0 || height <= 0) return;

	// Only resize if the dimensions actually changed
	if (width == viewportWidth && height == viewportHeight && FBO != 0) return;

	viewportWidth = width;
	viewportHeight = height;

	// If FBO doesn't exist, generate it
	if (FBO == 0) {
		glGenFramebuffers(1, &FBO);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// If a texture already exists, delete it before creating a new one
	if (textureColorbuffer != 0) {
		glDeleteTextures(1, &textureColorbuffer);
	}

	// 1. Generate the texture
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

	// 2. Create an empty texture with the new width and height
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// 3. Attach it to the FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

	// Unbind the FBO so we don't accidentally draw to it elsewhere
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}