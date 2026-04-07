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
	// glEnable(GL_PROGRAM_POINT_SIZE); // let the shaders set the point size with gl_PointSize
	// glEnable(0x8861); // Force-enable GL_POINT_SPRITE (0x8861)
	// glEnable(GL_MULTISAMPLE);// Enable antialising
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


void Renderer::Render(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera, const SimType& simType)
{
	passBufferLinks(simulation, iState, camera);
	passBufferNodes(simulation, iState, camera, simType);


	// BIND THE FBO (Intercept the draw calls)
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		// SET VIEWPORT TO MATCH FBO TEXTURE SIZE
		glViewport(0, 0, viewportWidth, viewportHeight);


		// Rendering commands here
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Dark teal background
		glClear(GL_COLOR_BUFFER_BIT);

		renderLinks(simulation, iState, camera);
		renderNodes(simulation, iState, camera);

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


void Renderer::initializeNodes()
{
	// --- Shaders ---
	// Vertex Shader
	const char* vertexShaderSource = R"(#version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in vec2 aUV;

	uniform mat4 uOrthoMatrix;
	
    out vec3 ourColor;
    out vec2 uv;
    void main()
    {
		// ORTHOGRAPHIC PROJECTION
		gl_Position = uOrthoMatrix * vec4(aPos, 0.0, 1.0);
        uv = aUV;
        ourColor = aColor;
    })";

	const char* fragmentShaderSource = R"(#version 330 core
    in vec3 ourColor;
	in vec2 uv;
    out vec4 FragColor;
    void main()
    {
        vec2 circCoord = uv - vec2(0.5, 0.5);// coordinates from center
		float dist = length(circCoord);		 // distance from center
        if(dist > 0.5) discard;
		
		float pixelThickness = fwidth(dist); // how big the pixel is compared to the square (depends on zoom)
		float alpha = 1.0 - smoothstep(0.5 - 2*pixelThickness, 0.5, dist);
		
        
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

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
			glEnableVertexAttribArray(2);
		// Unbind the VBO and VAO safely
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

static void pushCircle(std::vector<Vertex>* vertices, Vec2 center, float radius, RGB color){
	Vertex v1{ Vec2{center.x - radius, center.y - radius}, color, Vec2{0,0} }; // bottom left
	Vertex v2{ Vec2{center.x + radius, center.y - radius}, color, Vec2{1,0} }; // bottom right
	Vertex v3{ Vec2{center.x - radius, center.y + radius}, color, Vec2{0,1} }; // top left
	Vertex v4{ Vec2{center.x + radius, center.y + radius}, color, Vec2{1,1} }; // top right

	vertices->push_back(v1);
	vertices->push_back(v2);
	vertices->push_back(v3);

	vertices->push_back(v2);
	vertices->push_back(v3);
	vertices->push_back(v4);
}

void Renderer::passBufferNodes(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera, const SimType& simType)
{
	// 6 vertices per node
	std::vector<Vertex> vertices; vertices.reserve(simulation.getN() * 6);

	for (int node = 0; node < simulation.getN(); node++) {
		Vec2 worldPos = simulation.getNodePositions()[node];

		// Set color based on node state
		RGB color = { 1.0f, 1.0f, 1.0f }; //  white
		switch (simType) {
		case SIM_SIR:
			switch (simulation.getNodeState(node)) {
			case S: color = { 0.0f, 0.0f, 1.0f }; break; // blue
			case I: color = { 1.0f, 0.0f, 0.0f }; break; // red
			case R: color = { 0.0f, 1.0f, 0.0f }; break; // green
			}
		break;
		case SIM_KURAMOTO:
			float phase = simulation.getNodePhase(node);
			phase = (1 + std::cos(phase))*0.5;
			phase = phase * phase;
			phase = phase * phase; // ((1 + cosx) ^ 4)/2
			color = { phase, phase, 1.0f - phase }; // blue to yellow
			break;
		}
		if (iState.hoveredNodeId == node) {
			color = { 1.0f, 1.0f, 1.0f }; // white
		}
		pushCircle(&vertices, worldPos, simulation.getNodeRadius()[node], color);
	}

	// Copy our vertices array into a buffer for OpenGL to use
	// No need to bind the VAO, since the VBO already knows it uses the VAO for the instructions
	glBindBuffer(GL_ARRAY_BUFFER, nodeVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::renderNodes(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera)
{
	// Draw the triangle
	glUseProgram(nodeShaderProgram);

	// Send uniforms to the shader
	glUniformMatrix4fv(glGetUniformLocation(nodeShaderProgram, "uOrthoMatrix"), 1, GL_FALSE, camera.GetOrthoMatrix().data());

	glBindVertexArray(nodeVAO); // Bind the VAO containing our triangle configuration
	glDrawArrays(GL_TRIANGLES, 0, (int)simulation.getN() * 6); // represents the number of vertices to draw
	glBindVertexArray(0);
}

void Renderer::initializeLinks()
{
	// --- Shaders ---
	// Vertex Shader
	const char* vertexShaderSource = R"(#version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
	layout (location = 2) in vec2 aUV;
    
    uniform mat4 uOrthoMatrix;
    uniform float uZoom;

    out vec3 ourColor;
	out vec2 uv;
    
    void main()
    {
		// ORTHOGRAPHIC PROJECTION
		vec4 ndcPos = uOrthoMatrix * vec4(aPos, 0.0, 1.0);
        
        gl_Position = ndcPos; // Z is 0.0, W is 1.0
        uv = aUV;
        ourColor = aColor;
    })";

	const char* fragmentShaderSource = R"(#version 330 core
    in vec3 ourColor;
	in vec2 uv;
    out vec4 FragColor;
    void main()
    {	
		float distx = uv.x;
		float disty = uv.y;
		float pixelThicknessx = fwidth(distx);
		float pixelThicknessy = fwidth(disty);
		float alphax = 1 - smoothstep(1 - pixelThicknessx, 1.0, uv.x);
        float alphaxneg = smoothstep(0, pixelThicknessx, uv.x);
		float alphay = 1 - smoothstep(1 - pixelThicknessy, 1.0, uv.y);
		float alphayneg = smoothstep(0, pixelThicknessy, uv.y);
		FragColor = vec4(ourColor, alphax*alphaxneg*alphay*alphayneg);
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

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,pos));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
			glEnableVertexAttribArray(2);

		// Unbind the VBO and VAO safely
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

static void pushLine(std::vector<Vertex>* vertices, Vec2 worldPosA, Vec2 worldPosB, RGB color, float width) {
	// MATH FOR CALCULATING RECTANGLES
	Vec2 vector = worldPosB - worldPosA;
	float length = sqrt(vector * vector);
	Vec2 vectorUnit = vector / length;
	Vec2 normalUnit = Vec2{ -vectorUnit.y, vectorUnit.x };

	// shift the worldPosB and worldPosA by width/2 (width=6) in the normalUnit direction, both ways.
	//														  // UV COORDINATES							
	Vertex v1 = Vertex{worldPosA + normalUnit * width, color, Vec2{0.0f, 0.0f}};	 // worldPosA_1
	Vertex v2 = Vertex{worldPosA - normalUnit * width, color, Vec2{1.0f, 0.0f}};	 // worldPosA_2
	Vertex v3 = Vertex{worldPosB + normalUnit * width, color, Vec2{0.0f, 1.0f}};	 // worldPosB_1
	Vertex v4 = Vertex{worldPosB - normalUnit * width, color, Vec2{1.0f, 1.0f}};	 // worldPosB_2

	// These are the 4 vertices. Draw v1,v2,v3 and v2,v3,v4

	vertices->push_back(v1);
	vertices->push_back(v2);
	vertices->push_back(v3);

	vertices->push_back(v2);
	vertices->push_back(v3);
	vertices->push_back(v4);
}

void Renderer::passBufferLinks(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera)
{
	// each link is 6 vertices in size (2 triangles) * (3 vertices)
	// all the links AND THE ONE TO THE MOUSE
	std::vector<Vertex> vertices; vertices.reserve((simulation.getLinks().size() + 1) * 6);
	
	// 1 vertex per node, draw them as GL_POINTS


	for (const auto& link : simulation.getLinks()) {
		// Set color
		RGB color{1.0f, 1.0f, 1.0f}; //  white
		if (iState.hoveredLink == link) {
			color = { 1.0f, 0.0f, 0.0f }; // red
		}

		Vec2 worldPosA = simulation.getNodePositions()[link.nodeA];
		Vec2 worldPosB = simulation.getNodePositions()[link.nodeB];

		pushLine(&vertices, worldPosA, worldPosB, color, 3.0f * sqrt(std::min(link.weight, 4.0f)));
	}
	if (iState.draggedNodeId != -1) {
		RGB color{ 1.0f, 1.0f, 1.0f }; //  white
		Vec2 worldPosA = simulation.getNodePositions()[iState.draggedNodeId];
		Vec2 worldPosB = iState.worldMousePos;

		pushLine(&vertices, worldPosA, worldPosB, color, 3.0f);
	}

	// Copy our vertices array into a buffer for OpenGL to use
	// No need to bind the VAO, since the VBO already knows it uses the VAO for the instructions
	glBindBuffer(GL_ARRAY_BUFFER, linkVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::renderLinks(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera)
{
	// Draw the triangle
	glUseProgram(linkShaderProgram);

	// Send uniforms to the shader

	glUniformMatrix4fv(glGetUniformLocation(linkShaderProgram, "uOrthoMatrix"), 1, GL_FALSE, camera.GetOrthoMatrix().data());
	glUniform1f(glGetUniformLocation(linkShaderProgram, "uZoom"), camera.zoom);

	glBindVertexArray(linkVAO); // Bind the VAO containing our triangle configuration
		int nLinkstoDraw = (int)simulation.getLinks().size();
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