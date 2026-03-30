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
	

	// -----------------------------------------------------------------------------------------------
	// -------------------------ENABLE A BUNCH OF SHIT NEEDED TO WORK---------------------------------
	glEnable(GL_PROGRAM_POINT_SIZE); // let the shaders set the point size with gl_PointSize
	glEnable(0x8861); // Force-enable GL_POINT_SPRITE (0x8861)
	glEnable(GL_MULTISAMPLE);// Enable antialising
	// Enable alpha transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// -----------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------
	
	// set up shaders, VAO and VBO
	initializeNodes();
	initializeLinks();


	// Initialize FBO to a default size
	Resize(800, 600);

}


void Renderer::Render(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	passBufferLinks(simulation, iState, zoom, offset);
	passBufferNodes(simulation, iState, zoom, offset);


	// BIND THE FBO (Intercept the draw calls)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		// SET VIEWPORT TO MATCH FBO TEXTURE SIZE
		glViewport(0, 0, viewportWidth, viewportHeight);


		// Rendering commands here
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark teal background
		glClear(GL_COLOR_BUFFER_BIT);

		renderLinks(simulation, iState, zoom, offset);
		renderNodes(simulation, iState, zoom, offset);

		// 2. RESOLVE: Copy from Multisample FBO to the Screen (Default FBO)
		// This "flattens" the 4 samples per pixel into 1 smooth pixel
		glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // 0 is the screen
		glBlitFramebuffer(	0, 0, viewportWidth, viewportHeight,
							0, 0, viewportWidth, viewportHeight,
							GL_COLOR_BUFFER_BIT, GL_NEAREST);
		GLenum err = glGetError(); if (err != GL_NO_ERROR) std::cout << "GL Error: " << err << std::endl;
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

		// generate the texture
		glGenTextures(1, &textureColorbuffer);

		// without antialiasing: GL_TEXTURE_2D
		// with antialiasing: GL_TEXTURE_2D_MULTISAMPLE
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer);

			// create an empty texture with the new width and height
			
			
			// With antialiasing: glTexImage2DMultisample
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, GL_TRUE);
			
			// Without antialiasing:
			// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// attach it to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorbuffer, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	// Unbind the FBO so we don't accidentally draw to it elsewhere
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void Renderer::initializeNodes()
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
        vec2 circCoord = gl_PointCoord - vec2(0.5, 0.5);// coordinates from center
		float dist = length(circCoord);					// distance from center
		
		float pixelThickness = fwidth(dist); // how big the pixel is compared to the square (depends on zoom)
		float alpha = 1.0 - smoothstep(1 - 2*pixelThickness, 1.0, 2*dist);
		
        if(length(circCoord) > 0.5) discard;
		
        
        FragColor = vec4(ourColor, alpha);
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
	nodeShaderProgram = glCreateProgram();
	glAttachShader(nodeShaderProgram, vertexShader);
	glAttachShader(nodeShaderProgram, fragmentShader);
	glLinkProgram(nodeShaderProgram);

	// Delete the individual shaders as they're now linked into our program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	// THE VAO (Vertex Array Object) IS THE INSTRUCTIONS TO DRAW THE VERTICES
	// THE VBO (Vertex Buffer Object) IS THE RAW DATA, ALL THE FLOATS IN LINE
	glGenVertexArrays(1, &nodeVAO);
	glGenBuffers(1, &nodeVBO);

	// Bind the Vertex Array Object first, then bind and set vertex buffers

	glBindVertexArray(nodeVAO);

		// THE VAO NOW LOOKS FOR THE DATA IN THE VBO. THERE IS NO NEED TO BIND THE VBO ANYMORE, ONLY THE VAO WHEN WE USE IT
		glBindBuffer(GL_ARRAY_BUFFER, nodeVBO);

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
}

void Renderer::passBufferNodes(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// Create the array of nodes positions and colors to send to the GPU
	std::vector<float> vertices; vertices.resize(simulation.getN() * 5); // 5 floats per vertex (2position + 3color)
	// 1 vertex per node, draw them as GL_POINTS
	for (int node = 0; node < simulation.getN(); node++) {
		Vec2 worldPos = simulation.getNodePositions()[node];

		vertices[node * 5 + 0] = worldPos.x;
		vertices[node * 5 + 1] = worldPos.y;

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


	// Copy our vertices array into a buffer for OpenGL to use
	// No need to bind the VAO, since the VBO already knows it uses the VAO for the instructions
	glBindBuffer(GL_ARRAY_BUFFER, nodeVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::renderNodes(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// Draw the triangle
	glUseProgram(nodeShaderProgram);

	// Send uniforms to the shader

	glUniform2f(glGetUniformLocation(nodeShaderProgram, "uViewportSize"), (float)viewportWidth, (float)viewportHeight);
	glUniform2f(glGetUniformLocation(nodeShaderProgram, "uOffset"), offset.x, offset.y);
	glUniform1f(glGetUniformLocation(nodeShaderProgram, "uZoom"), zoom);
	glUniform1f(glGetUniformLocation(nodeShaderProgram, "uNodeSize"), 20.0f);

	glBindVertexArray(nodeVAO); // Bind the VAO containing our triangle configuration
	glDrawArrays(GL_POINTS, 0, simulation.getN()); // represents the number of vertices to draw
	glBindVertexArray(0);
}

void Renderer::initializeLinks()
{
	// --- Shaders ---
	// Vertex Shader
	const char* vertexShaderSource = R"(#version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    
    uniform vec2 uViewportSize;
    uniform vec2 uOffset;
    uniform float uZoom;

    out vec3 ourColor;
    
    void main()
    {
        // Convert world position to screen pixels
        vec2 screenPos = (aPos * uZoom) + uOffset;
        
        // Convert screen pixels to NDC [-1.0, 1.0]
        vec2 ndcPos = (screenPos / uViewportSize) * 2.0 - 1.0;
        ndcPos.y = -ndcPos.y; // Flip OpenGL's Y axis
        
        gl_Position = vec4(ndcPos, 0.0, 1.0); // Z is 0.0, W is 1.0
        
        ourColor = aColor;
    })";

	const char* fragmentShaderSource = R"(#version 330 core
    in vec3 ourColor;
    out vec4 FragColor;
    void main()
    {
        //   vec2 circCoord = gl_PointCoord - vec2(0.5, 0.5);// coordinates from center
		//   float dist = length(circCoord);					// distance from center
		//   
		//   float pixelThickness = fwidth(dist); // how big the pixel is compared to the square (depends on zoom)
		//   float alpha = 1.0 - smoothstep(1 - 2*pixelThickness, 1.0, 2*dist);
		//   
        //   if(length(circCoord) > 0.5) discard;
		
        
        FragColor = vec4(ourColor, 1.0);
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
	linkShaderProgram = glCreateProgram();
	glAttachShader(linkShaderProgram, vertexShader);
	glAttachShader(linkShaderProgram, fragmentShader);
	glLinkProgram(linkShaderProgram);

	// Delete the individual shaders as they're now linked into our program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);


	// THE VAO (Vertex Array Object) IS THE INSTRUCTIONS TO DRAW THE VERTICES
	// THE VBO (Vertex Buffer Object) IS THE RAW DATA, ALL THE FLOATS IN LINE
	glGenVertexArrays(1, &linkVAO);
	glGenBuffers(1, &linkVBO);

	// Bind the Vertex Array Object first, then bind and set vertex buffers

	glBindVertexArray(linkVAO);

		// THE VAO NOW LOOKS FOR THE DATA IN THE VBO. THERE IS NO NEED TO BIND THE VBO ANYMORE, ONLY THE VAO WHEN WE USE IT
		glBindBuffer(GL_ARRAY_BUFFER, linkVBO);

			// TELL OPENGL HOW THE DATA IS STRUCTURED
			// ARGUMENTS: (index, size, type, normalized, stride, pointer)

			// For links, only pass triangles and their color, a 2d float and a 3d color per vertex

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
			glEnableVertexAttribArray(1);

		// Unbind the VBO and VAO safely
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void pushLinktoVertices(std::vector<float>* vertices, Vec2 worldPosA, Vec2 worldPosB, float r, float g, float b) {
	// MATH FOR CALCULATING RECTANGLES
	Vec2 vector = worldPosB - worldPosA;
	float length = sqrt(vector * vector);
	Vec2 vectorUnit = vector / length;
	Vec2 normalUnit = Vec2{ -vectorUnit.y, vectorUnit.x };

	// shift the worldPosB and worldPosA by width/2 (width=4) in the normalUnit direction, both ways.
	Vec2 v1 = worldPosA + normalUnit * 2.0f;	 // worldPosA_1
	Vec2 v2 = worldPosA - normalUnit * 2.0f;	 // worldPosA_2
	Vec2 v3 = worldPosB + normalUnit * 2.0f;	 // worldPosB_1
	Vec2 v4 = worldPosB - normalUnit * 2.0f;	 // worldPosB_2

	// These are the 4 vertices. Draw v1,v2,v3 and v2,v3,v4

	vertices->push_back(v1.x);
	vertices->push_back(v1.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
	vertices->push_back(v2.x);
	vertices->push_back(v2.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
	vertices->push_back(v3.x);
	vertices->push_back(v3.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
	vertices->push_back(v2.x);
	vertices->push_back(v2.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
	vertices->push_back(v3.x);
	vertices->push_back(v3.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
	vertices->push_back(v4.x);
	vertices->push_back(v4.y);
	vertices->push_back(r);
	vertices->push_back(g);
	vertices->push_back(b);
}

void Renderer::passBufferLinks(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// 30 floats per link (2 triangles) * (3 vertices) * (2position + 3color)
	// all the links AND THE ONE TO THE MOUSE
	std::vector<float> vertices; vertices.reserve((simulation.getLinks().size() + 1) * 30);
	
	// 1 vertex per node, draw them as GL_POINTS

	for (const auto& link : simulation.getLinks()) {
		// Set color
		float r = 1.0f, g = 1.0f, b = 1.0f; //  white
		if (iState.hoveredLink == link) {
			r = 1.0f; g = 0.0f; b = 0.0f; // red
		}

		Vec2 worldPosA = simulation.getNodePositions()[link.nodeA];
		Vec2 worldPosB = simulation.getNodePositions()[link.nodeB];

		pushLinktoVertices(&vertices, worldPosA, worldPosB, r, g, b);
	}
	if (iState.draggedNodeId != -1) {
		float r = 1.0f, g = 1.0f, b = 1.0f; //  white
		Vec2 worldPosA = simulation.getNodePositions()[iState.draggedNodeId];
		Vec2 worldPosB = iState.worldMousePos;

		pushLinktoVertices(&vertices, worldPosA, worldPosB, r, g, b);
	}

	// Copy our vertices array into a buffer for OpenGL to use
	// No need to bind the VAO, since the VBO already knows it uses the VAO for the instructions
	glBindBuffer(GL_ARRAY_BUFFER, linkVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::renderLinks(const GraphSimulation& simulation, const InteractionState& iState, float zoom, Vec2 offset)
{
	// Draw the triangle
	glUseProgram(linkShaderProgram);

	// Send uniforms to the shader

	glUniform2f(glGetUniformLocation(linkShaderProgram, "uViewportSize"), (float)viewportWidth, (float)viewportHeight);
	glUniform2f(glGetUniformLocation(linkShaderProgram, "uOffset"), offset.x, offset.y);
	glUniform1f(glGetUniformLocation(linkShaderProgram, "uZoom"), zoom);

	glBindVertexArray(linkVAO); // Bind the VAO containing our triangle configuration
		int nLinkstoDraw = simulation.getLinks().size();
		if (iState.draggedNodeId != -1)
			nLinkstoDraw += 1;
		glDrawArrays(GL_TRIANGLES, 0, nLinkstoDraw * 6); // represents the number of vertices to draw (6 per link)
	glBindVertexArray(0);
}

Renderer::~Renderer()
{
	// Clean up nodes resources
	glDeleteVertexArrays(1, &nodeVAO);
	glDeleteBuffers(1, &nodeVBO);
	glDeleteProgram(nodeShaderProgram);

	// Clean up links resources
	glDeleteVertexArrays(1, &linkVAO);
	glDeleteBuffers(1, &linkVBO);
	glDeleteProgram(linkShaderProgram);
}