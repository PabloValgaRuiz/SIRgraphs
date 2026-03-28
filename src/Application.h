#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GraphSimulation.h"

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <random>
#include "pcg_random.hpp"


struct InteractionState {
    Vec2 worldMousePos{};

    int hoveredNodeId = -1;
    int draggedNodeId = -1;
    Link hoveredLink{};
};

class MyApp {
public:
    MyApp();
    void run();
    void render();
    void UpdateViewportCamera();
    void HandleInput();
    void ParameterWindowUI();

private:
    ImVec4 setNodeColor(int node, int hoveredNodeId);
    void drawInfectedPlot() const;

    GraphSimulation simulation{};
    InteractionState iState{};
    bool isSimulationPlaying = false;
	float deltaTimeAccumulator = 0.0f;

    
    
    Vec2 canvas_p0{};
    Vec2 viewportSize = Vec2{800.0f, 600.0f};
    float worldSpaceZoom = 1.0f;
    Vec2 worldSpaceOffset = Vec2{400.0f, 300.0f};


    // Switch between creating network and infecting nodes
    int isInfectionMode = 0;
};
