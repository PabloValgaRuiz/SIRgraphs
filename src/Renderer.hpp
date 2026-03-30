#pragma once
// Renderer using openGL to render to texture and then display that texture in the ImGui window

#include "GraphSimulation.h"


class Renderer{
public:
	Renderer();
	//void Resize(int width, int height);
	void Render(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera);

	void Resize(int width, int height);
	unsigned int getTextureID() const { return textureColorbuffer; }

	~Renderer();
private:

	// vertex buffer object, vertex array object, shader program
	unsigned int nodeVBO, nodeVAO;
	unsigned int nodeShaderProgram;
	void initializeNodes();
	void passBufferNodes(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera);
	void renderNodes(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera);

	unsigned int linkVBO, linkVAO;
	unsigned int linkShaderProgram;
	void initializeLinks();
	void passBufferLinks(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera);
	void renderLinks(const GraphSimulation& simulation, const InteractionState& iState, const Camera2D& camera);

	// variables for off-screen rendering
	unsigned int FBO = 0;
	unsigned int textureColorbuffer = 0;
	int viewportWidth = 800;
	int viewportHeight = 800;
};