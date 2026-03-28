#include "Application.h"
#include "GraphSimulation.h"
#include <random>


MyApp::MyApp()
{
    ImGui::GetIO().FontGlobalScale = 1.3f;

}



ImVec4 MyApp::setNodeColor(int node, int hoveredNodeId) {

    if (node == hoveredNodeId)
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    switch (simulation.getNodeState(node)) {
    case S:
        return ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    case I:
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    case R:
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

static ImVec2 worldToScreenTransform(Vec2 worldCoord, Vec2 canvas_p0 = Vec2{0,0}, float zoom = 1.0f, Vec2 offset = Vec2{0,0}) {
    return ImVec2(  (worldCoord.x * zoom) + canvas_p0.x + offset.x,
                    (worldCoord.y * zoom) + canvas_p0.y + offset.y);
}
static Vec2 screenToWorldTransform(ImVec2 screenCoord, Vec2 canvas_p0 = Vec2{ 0,0 }, float zoom = 1.0f, Vec2 offset = Vec2{ 0,0 }) {
    return Vec2{ (screenCoord.x - canvas_p0.x - offset.x) / zoom,
                 (screenCoord.y - canvas_p0.y - offset.y) / zoom };
}
// links


void MyApp::run() {

    // Calculate time
    float deltaTime = ImGui::GetIO().DeltaTime;
    if (deltaTime > 0.04f) deltaTime = 0.04f;

    // ImGui dockspace setup

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(0, viewport);

    // Run simulations
    
    deltaTimeAccumulator += deltaTime;
    while(deltaTimeAccumulator >= 1.0/75) {
        simulation.UpdatePhysics(1.0 / 75);
        if (isSimulationPlaying) {
            simulation.UpdateSIR(1.0 / 75);
        }
        deltaTimeAccumulator -= 1.0 / 75;
    }

    // Draw the UI panels and buttons
    ParameterWindowUI();

    // create a new ImGui window called "Simulation Viewport"
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Viewport", nullptr);


	// Handle user input for interacting with the simulation (creating nodes and links, dragging nodes, zooming, etc.)
    UpdateViewportCamera();
    HandleInput();

    // Render the visuals in the simulation viewport
    render();

    ImGui::End();

    //ImGui::ShowDemoWindow();
}

void MyApp::render()
{   
    auto& position = simulation.getNodePositions();
	auto& radius = simulation.getNodeRadius();
    auto& links = simulation.getLinks();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw links and nodes
    for (const auto& link : links) {

        ImVec2 screenPosA = worldToScreenTransform(position[link.nodeA], canvas_p0, worldSpaceZoom, worldSpaceOffset);
        ImVec2 screenPosB = worldToScreenTransform(position[link.nodeB], canvas_p0, worldSpaceZoom, worldSpaceOffset);

        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
        if (iState.hoveredLink == link) color = ImVec4(1.0, 0.0, 0.0, 1.0);
        draw_list->AddLine(screenPosA, screenPosB, ImGui::GetColorU32(color), 4.0f * worldSpaceZoom);
    }
    for (int node = 0; node < position.size(); node++){
        ImVec2 screenPos = worldToScreenTransform(position[node], canvas_p0, worldSpaceZoom, worldSpaceOffset);

        ImVec4 color = setNodeColor(node, iState.hoveredNodeId);
        draw_list->AddCircleFilled(screenPos, radius[node] * worldSpaceZoom, ImGui::GetColorU32(color));

    }

    // DRAW THE LINE BEING DRAGGED FROM NODE TO MOUSE
    if (iState.draggedNodeId != -1) {
        auto draggedPosition = position[iState.draggedNodeId];
        auto screenDraggedPosition = worldToScreenTransform(draggedPosition, canvas_p0, worldSpaceZoom, worldSpaceOffset);
        ImVec2 mousePos = worldToScreenTransform(iState.worldMousePos, canvas_p0, worldSpaceZoom, worldSpaceOffset);
        draw_list->AddLine(screenDraggedPosition, mousePos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)), 4.0f * worldSpaceZoom);
    }

    char textInfo[64];
    sprintf(textInfo, "Nodes: %i\nLinks: %i", (int)position.size(), (int)links.size());
    ImGui::Text(textInfo);
}

void MyApp::UpdateViewportCamera() {
    viewportSize = Vec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
    
    worldSpaceOffset = Vec2{ viewportSize.x / 2, viewportSize.y / 2 };
    // get coordinates of the top-left corner of the window to know the absolute coordinates to draw
    canvas_p0 = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };

	// if the window is hovered and we are dragging, update the world space offset to move the camera
    if (ImGui::IsWindowHovered() && iState.draggedNodeId == -1 && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        worldSpaceOffset.x += ImGui::GetIO().MouseDelta.x;
        worldSpaceOffset.y += ImGui::GetIO().MouseDelta.y;
	}

}

void MyApp::HandleInput(){

    // GET MOUSE POSITION AND SET HOVERING STATE
    ImVec2 mousePos = ImGui::GetMousePos();
    iState.worldMousePos = screenToWorldTransform(mousePos, canvas_p0, worldSpaceZoom, worldSpaceOffset);


    if (ImGui::IsWindowHovered()) {

        // ZOOM

        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            // Optional: Zoom towards mouse position
            worldSpaceZoom += wheel * 0.1f;
            if (worldSpaceZoom < 0.1f) worldSpaceZoom = 0.1f; // Cap zoom
        }

        iState.hoveredNodeId = simulation.GetHoveredNodeId(iState.worldMousePos);
        iState.hoveredLink = simulation.GetHoveredLink(iState.worldMousePos);

        if (isInfectionMode == 1) {
            // if we are in infection mode, left clicking on a node infects it and right clicking on a node makes it susceptible
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (iState.hoveredNodeId != -1) {
                    simulation.SetNodeState(iState.hoveredNodeId, I);
                }
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (iState.hoveredNodeId != -1) {
                    simulation.SetNodeState(iState.hoveredNodeId, S);
                }
            }
        }
        else if (isInfectionMode == 0) {
            // delete a node where you right click if there is a node there
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (iState.hoveredNodeId != -1) {
                    simulation.removeNode(iState.hoveredNodeId);
                    iState.hoveredNodeId = -1;
                }
                else if (iState.hoveredLink.nodeA != -1) {
                    simulation.removeLink(iState.hoveredLink.nodeA, iState.hoveredLink.nodeB);
                }
            }

            // LEFT CLICK PRESSED
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // add a link between two nodes when left clicking and dragging from one node to another
                if (iState.hoveredNodeId != -1) {
                    iState.draggedNodeId = iState.hoveredNodeId;
                }
                // add a node where you left click and there isn't already a node there
                else {
                    simulation.addNode(Vec2{ iState.worldMousePos.x, iState.worldMousePos.y });
                }
            }
            // LEFT CLICK RELEASED
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // if we were dragging from a node and we release over another node, add a link between them
                if (iState.draggedNodeId != -1 && iState.hoveredNodeId != -1)
                    if (iState.draggedNodeId != iState.hoveredNodeId)
                        simulation.addLink(iState.draggedNodeId, iState.hoveredNodeId);
                iState.draggedNodeId = -1;

            }
        }
    }

}

void MyApp::ParameterWindowUI(){

    ImGui::Begin("Parameters window");
    ImGui::PushItemWidth(100.0f);

    static int numNodesER = 20;
    static float pER = 0.05f;

    if (ImGui::Button("Create Erdos-Renyi graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.createRandomGraph(numNodesER, pER, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (ER)", &numNodesER, 0.25f, 1, 100);
    ImGui::DragFloat("Edge probability (ER)", &pER, 0.0025f, 0.0f, 1.0f, "%.2f");

    static int numNodesBA = 20;
    static int k = 1;

    if (ImGui::Button("Create Barabasi-Albert graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.createBarabasiAlbertGraph(numNodesBA, k, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (BA)", &numNodesBA, 0.125f, 1, 100);
    ImGui::DragInt("Links per node (BA)", &k, 0.05f, 1, 10);

    ImGui::PopItemWidth();

    if (ImGui::Button("Add Erdos-Renyi node", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        if (ImGui::GetIO().KeyShift) {
            // Shift is currently held down
            for (int i = 0; i < 10; i++) {
                simulation.addNodeErdosRenyi(pER);
            }
        }
        else
            simulation.addNodeErdosRenyi(pER);
    }
    if (ImGui::Button("Add Barabasi-Albert node", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        if (ImGui::GetIO().KeyShift) {
            // Shift is currently held down
            for (int i = 0; i < 10; i++) {
                simulation.addNodeBarabasiAlbert(k);
            }
        }
        else
            simulation.addNodeBarabasiAlbert(k);
    }

    if (ImGui::Button("Delete graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.deleteGraph();
    }

    ImGui::PushItemWidth(100.0f);

    ImGui::DragFloat("Repulsion force", &simulation.repulsionForce, 10000.0f, 10000.0f, 20000000.0f, "%.0f");
    ImGui::DragFloat("Spring Length", &simulation.springLength, 0.5f, 5.0f, 500.0f, "%.0f");
    ImGui::DragFloat("Spring Stiffness", &simulation.springStiffness, 1.0f, 25.0f, 2500.0f, "%.0f");
    ImGui::DragFloat("Damping", &simulation.damping, 0.001f, 0.5f, 1.0f, "%.2f");
    ImGui::DragFloat("Central gravity", &simulation.centralGravity, 2.0f, 1.0f, 3000.0f, "%.0f");

    ImGui::PopItemWidth();

    if (!isSimulationPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.9f, 0.1f, 1.0f));
        if (ImGui::Button("PLAY SIMULATION", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
            isSimulationPlaying = true;
        ImGui::PopStyleColor();
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.0f, 1.0f));
        if (ImGui::Button("PAUSE SIMULATION", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
            isSimulationPlaying = false;
        ImGui::PopStyleColor();
    }

    ImGui::RadioButton("SIR", &simulation.epidemicType, 0); ImGui::SameLine();
    ImGui::RadioButton("SIS", &simulation.epidemicType, 1);

    ImGui::PushItemWidth(100.0f);
    ImGui::DragFloat("Infection rate", &simulation.lambdaInfection, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Recovery rate", &simulation.muRecovery, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    // Switch between creating network and infecting nodes
    ImGui::RadioButton("Create network", &isInfectionMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Infect nodes", &isInfectionMode, 1);

    if (ImGui::Button("Recover all nodes", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.RecoverAll();
    }

    drawInfectedPlot();

    ImGui::End();
}

void MyApp::drawInfectedPlot() const{
    // Fill an array of contiguous float values to plot
    // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
    // and the sizeof() of your structure in the "stride" parameter.

    static float values[180] = {};
    static int values_offset = 0;
    static double refresh_time = ImGui::GetTime();
    static float maxValue = 5;
    if (!isSimulationPlaying)
        refresh_time = ImGui::GetTime();
    while (refresh_time < ImGui::GetTime() && isSimulationPlaying) // Create data at fixed 60 Hz rate for the demo
    {
        float infectedCount = 0;
        for( int node = 0; node < simulation.getN(); node++){
            if (simulation.getNodeState(node) == I) infectedCount += 1;
        }

        values[values_offset] = infectedCount;
        values_offset = (values_offset + 1) % IM_COUNTOF(values);
        refresh_time += 1.0f / 10.0f;

        maxValue = 5;
        for (auto value : values) {
            if (value > maxValue) maxValue = value;
        }
    }

    ImGui::PushItemWidth(400.0f);
    ImGui::PlotLines("I", values, IM_COUNTOF(values), values_offset, 0, 0.0f, maxValue * 1.2f, ImVec2(0.0f, 200.0f));
    ImGui::PopItemWidth();
}
