#include "Renderer.hpp"

#include <iostream>
//include glad before glfw
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void CheckShaderError(unsigned int shader, const std::string& type) {
	int success;
	char infoLog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
		else{
			std::cout << "Shader " << type << " compiled successfully." << std::endl;
		}
	}
	else {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
		else{
			std::cout << "Shader " << type << " compiled successfully." << std::endl;
		}
	}
}

Renderer::Renderer()
{
	// --- Shaders ---
	// Vertex Shader
	const char* vertexShaderSource = R"(#version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    
    uniform vec2 uViewportSize;
    uniform vec2 uOffset;
    uniform float uZoom;
    uniform float uNodeSize;

    out vec3 ourColor;
    
    void main()
    {
        // Convert world position to screen pixels
        vec2 screenPos = (aPos * uZoom) + uOffset;
        
        // Convert screen pixels to NDC [-1.0, 1.0]
        vec2 ndcPos = (screenPos / uViewportSize) * 2.0 - 1.0;
        ndcPos.y = -ndcPos.y; // Flip OpenGL's Y axis
        
        // Scale the point size based on the zoom level 
        gl_PointSize = 2 * uNodeSize * uZoom;
        gl_Position = vec4(ndcPos, 0.0, 1.0); // Z is 0.0, W is 1.0
        
        ourColor = aColor;
    })";

	const char* fragmentShaderSource = R"(#version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main()
    {
        vec2 centerAdjusted = gl_PointCoord - vec2(0.5, 0.5);
        if(length(centerAdjusted) > 0.5) discard; 
        
        FragColor = vec4(ourColor, 1.0f);
    })";
 
	//Build and Compile our Shader Program
	// Compile Vertex Shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	CheckShaderError(vertexShader, "VERTEX");

	// Compile Fragment Shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	CheckShaderError(fragmentShader, "FRAGMENT");

	// Link shaders into a Shader Program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	// Delete the individual shaders as they're now linked into our program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glEnable(GL_PROGRAM_POINT_SIZE); // let the shaders set the point size with gl_PointSize
	// THE DRIVER HACKS:
	glEnable(0x8861); // Force-enable GL_POINT_SPRITE (0x8861)
	// Force the driver to recalculate the origin coordinate, which often wakes up the gl_PointCoord generator
	glPointParameteri(0x8CA0, 0x8CA1); // GL_POINT_SPRITE_COORD_ORIGIN (0x8CA0) to GL_LOWER_LEFT (0x8CA1)

	// 6. Set up Buffers (VBO) and Arrays (VAO)

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Bind the Vertex Array Object first, then bind and set vertex buffers

	glBindVertexArray(VAO);

	// Copy our vertices array into a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	// TELL OPENGL HOW THE DATA IS STRUCTURED
	// ARGUMENTS: (index, size, type, normalized, stride, pointer)
	// index: label for each attribute (0, 1, 2...), no matter the size, reference in the vertex shader as "layout (location = index)"
	// size: number of components for the attribute (e.g. 3 for vec3, 1 for float)
	// type: data type of each component (e.g. GL_FLOAT, GL_INT, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT)
	// normalized: if false, used directly. if true, they are normalized to [-1,1] or [0,1]
	// stride: how big in bytes are ALL THE ATTRIBUTES of each vertex added.
	// pointer: how many bytes into the buffer to find the attribute (e.g. if the second attribute comes after a vec3, pointer would be 3 * sizeof(float))
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);


	// Unbind the VBO and VAO safely
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	// Initialize FBO to a default size
	Resize(800, 600);

}


void Renderer::Render(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// Create the array of nodes positions and colors to send to the GPU
	std::vector<float> vertices; vertices.resize(simulation.getN() * 5); // 5 floats per vertex (2position + 3color)
																		 // 1 vertex per node, draw them as GL_POINTS
	for (int node = 0; node < simulation.getN(); node++) {
		Vec2 worldPos = simulation.getNodePositions()[node];

		vertices[node * 5 + 0] = worldPos.x;
		vertices[node * 5 + 1] = worldPos.y;
		vertices[node * 5 + 2] = 0.0f; // z coordinate for 2D rendering
		
		// Set color based on node state
		float r = 1.0f, g = 1.0f, b = 1.0f; //  white
		switch (simulation.getNodeState(node)) {
		case S: r = 0.0f; g = 0.0f; b = 1.0f; break; // blue
		case I: r = 1.0f; g = 0.0f; b = 0.0f; break; // red
		case R: r = 0.0f; g = 1.0f; b = 0.0f; break; // green
		}
		if (iState.hoveredNodeId == node) {
			r = 1.0f; g = 1.0f; b = 1.0f; // white
		}

		vertices[node * 5 + 2] = r;
		vertices[node * 5 + 3] = g;
		vertices[node * 5 + 4] = b;
	}

	// Bind the VAO and VBO to draw the triangle

	glBindVertexArray(VAO);

	// Copy our vertices array into a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

	// Unbind the VBO and VAO safely
	glBindBuffer(GL_ARRAY_BUFFER, 0);



	// BIND THE FBO (Intercept the draw calls)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// SET VIEWPORT TO MATCH FBO TEXTURE SIZE
	glViewport(0, 0, viewportWidth, viewportHeight);


	// Rendering commands here
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark teal background
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw the triangle
	glUseProgram(shaderProgram);

	// Send uniforms to the shader

	glUniform2f(glGetUniformLocation(shaderProgram, "uViewportSize"), (float)viewportWidth, (float)viewportHeight);
	glUniform2f(glGetUniformLocation(shaderProgram, "uOffset"), offset.x, offset.y);
	glUniform1f(glGetUniformLocation(shaderProgram, "uZoom"), zoom);
	glUniform1f(glGetUniformLocation(shaderProgram, "uNodeSize"), 20.0f);

	glBindVertexArray(VAO); // Bind the VAO containing our triangle configuration
	glDrawArrays(GL_POINTS, 0, simulation.getN()); // represents the number of vertices to draw

	// UNBIND THE FBO (Return to drawing to the screen)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(0);
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


Renderer::~Renderer()
{
	// 8. Clean up resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);
}