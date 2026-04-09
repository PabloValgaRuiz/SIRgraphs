#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "2DUtils.h"

#include "GraphSimulation.h"
#include "Renderer.hpp"

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <random>
#include "pcg_random.hpp"




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

    // Camera coordinates
    Camera2D camera;
    int numberOfFrames{};

    // RENDERING
    Renderer renderer;

    // SIMULATION
    GraphSimulation simulation{};
    InteractionState iState{};
	bool isPhysicsPlaying = true;
    bool isSimulationPlaying = false;
	float deltaTimeAccumulator = 0.0f;

    // Switch between creating network and infecting nodes and info mode
    int isCreationMode = 0;

    // Pick dynamics to display and affect
    SimType simType = SIM_SIR;

    int infoPanelNode = -1;
    Link infoPanelLink{};
};
