#include "Application.h"
#include "GraphSimulation.h"
#include <random>

// For building the docking layout
#include <imgui_internal.h>

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

// links

static void ImGUIDockSetup(){
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, viewport);

    static bool first_time = true;
    if (first_time)
    {
        first_time = false;

        // Clear any previous layout
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // Split the dockspace
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 400.0f / 1400, nullptr, &dock_main_id);

        // Dock the windows into their respective areas.
        // Make sure to replace "Parameters" with whatever window title you use inside ParameterWindowUI()
        ImGui::DockBuilderDockWindow("Simulation Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Parameters window", dock_right_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }
    // -----------------------------------
}

void MyApp::run() {

    // Calculate time
    float deltaTime = ImGui::GetIO().DeltaTime;
    if (deltaTime > 0.04f) deltaTime = 0.04f;

    // ImGui dockspace setup -> create a dockspace with the viewport and the parameter window
    ImGUIDockSetup();

    // Draw the UI panels and buttons
    ParameterWindowUI();

    // create a new ImGui window called "Simulation Viewport"
    //ImGui::SetNextWindowSize(ImVec2(800, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Viewport", nullptr);


	// Handle user input for interacting with the simulation (creating nodes and links, dragging nodes, zooming, etc.)
    UpdateViewportCamera();
    HandleInput();


    // Run simulations
    deltaTimeAccumulator += deltaTime;
    while (deltaTimeAccumulator >= 1.0 / 75) {
		if (isPhysicsPlaying) {
            simulation.UpdatePhysics(1.0f / 75);
        }
        if (isSimulationPlaying) {
            switch (simType) {
            case SIM_SIR:
                simulation.UpdateSIR(1.0f / 75);
                deltaTimeAccumulator -= 1.0f / 75;
                break;
            case SIM_KURAMOTO:
                simulation.updateKuramoto(0.25f / 75);
                deltaTimeAccumulator -= 0.25f / 75;
                break;
            }
        }
        else deltaTimeAccumulator -= 1.0f / 75;
    }


    // render in the simulation viewport
    
    renderer.Resize((int)camera.viewportSize.x, (int)camera.viewportSize.y);
    renderer.Render(simulation, iState, camera, simType);
    uint32_t textureID = renderer.getTextureID();
    ImGui::Image(
        (ImTextureID)(intptr_t)textureID,
        ImVec2(camera.viewportSize.x, camera.viewportSize.y),
        ImVec2(0, 1), // Top-left UV
        ImVec2(1, 0)  // Bottom-right UV
    );

    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // SHOW THE LABELS OF THE NODES
    if (isCreationMode == 2) {
        for (int i = 0; i < simulation.getNodePositions().size(); i++) {
            ImGui::PushFont(NULL, 24);
            const std::string& text = simulation.getNodeLabels()[i];
            ImGui::PopFont();
            ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            ImVec2 textCenter = camera.WorldToScreen(simulation.getNodePositions()[i] + Vec2{ 0, 0 });
            ImVec2 textPos = ImVec2{ textCenter.x - textSize.x * 0.65f, textCenter.y - textSize.y*0.65f};
            //ImGui::Text(text.c_str());
            draw_list->AddText(NULL, 24, textPos, IM_COL32(200, 200, 200, 255), text.c_str());

        }
    }


    // DEBUG ---------------------------------------------
	// set the cursor position to the top-left corner of the window to draw text there
	ImGui::SetCursorPos(ImVec2(10, 30));
    // display text info on top
    char textInfo[32];
    snprintf(textInfo, sizeof(textInfo), "Nodes: %i\nLinks: %i", (int)simulation.getN(), (int)simulation.getLinks().size());
    ImGui::Text(textInfo);

#ifndef NDEBUG
    char debugInfo[256];
    snprintf(debugInfo, sizeof(debugInfo),
        "width: %i\nheight: %i\noffsetx: %i\noffsety: %i\ndisplacementx: %i\ndisplacementy: %i\nzoom: %f\n\n\nW_mouse_x: %f\nW_mouse_y: %f\nS_mouse_y: %f\nS_mouse_y: %f",
        (int)camera.viewportSize.x, (int)camera.viewportSize.y,
        (int)camera.offset.x, (int)camera.offset.y,
        (int)camera.displacement.x, (int)camera.displacement.y,
        camera.zoom, iState.worldMousePos.x, iState.worldMousePos.y,
        camera.WorldToScreen(iState.worldMousePos).x, camera.WorldToScreen(iState.worldMousePos).y);
    ImGui::Text(debugInfo);

    // ID OF THE NODE
    if (iState.hoveredNodeId != -1) {
        ImGui::SetCursorPos(camera.WorldToScreenNoP0(iState.worldMousePos));
        char nodeIDtext[32];
        snprintf(nodeIDtext, sizeof(nodeIDtext), "ID: %i", (int)iState.hoveredNodeId);
        ImGui::Text(nodeIDtext);
    }
#endif

    
    // ---------------------------------------------
    ImGui::End();

    // NODE DATA WHEN IN NETWORK DATA MODE
    if (isCreationMode == 2) {
        // new window
        if (infoPanelNode != -1) {
            static int oldPanelNode = -1;
            if(oldPanelNode != infoPanelNode)
                ImGui::SetNextWindowPos(camera.WorldToScreen(simulation.getNodePositions()[infoPanelNode]));
            oldPanelNode = infoPanelNode;
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
            ImGui::Begin("Info window");
            static char label[32] = "";
            auto& nodeLabel = simulation.getNodeLabels()[infoPanelNode];
            snprintf(label, sizeof(label), nodeLabel.c_str());
            ImGui::InputTextWithHint("Node label", "Enter label", label, IM_COUNTOF(label));
            nodeLabel = label;
            ImGui::End();
        }
        if (infoPanelLink.nodeA != -1) {
            static Link oldPanelLink = Link{};
            if (oldPanelLink != infoPanelLink)
                ImGui::SetNextWindowPos(camera.WorldToScreen(iState.worldMousePos));
            oldPanelLink = infoPanelLink;
            ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);
            ImGui::Begin("Info window");

            float& weight = simulation.getLinks().find(infoPanelLink)->weight;
            ImGui::InputFloat("Link weight", &weight, 0.0f, 0.0f, "%.2f");

            ImGui::End();
        }
    }
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

        ImVec2 screenPosA = camera.WorldToScreen(position[link.nodeA]);
        ImVec2 screenPosB = camera.WorldToScreen(position[link.nodeB]);

        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
        if (iState.hoveredLink == link) color = ImVec4(1.0, 0.0, 0.0, 1.0);
        draw_list->AddLine(screenPosA, screenPosB, ImGui::GetColorU32(color), 4.0f * camera.zoom);
    }
    for (int node = 0; node < position.size(); node++){
        ImVec2 screenPos = camera.WorldToScreen(position[node]);

        ImVec4 color = setNodeColor(node, iState.hoveredNodeId);
        draw_list->AddCircleFilled(screenPos, radius[node] * camera.zoom, ImGui::GetColorU32(color));

    }

    // DRAW THE LINE BEING DRAGGED FROM NODE TO MOUSE
    if (iState.draggedNodeId != -1) {
        auto draggedPosition = position[iState.draggedNodeId];
        auto screenDraggedPosition = camera.WorldToScreen(draggedPosition);
        ImVec2 mousePos = camera.WorldToScreen(iState.worldMousePos);
        draw_list->AddLine(screenDraggedPosition, mousePos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)), 4.0f * camera.zoom);
    }

    
}

void MyApp::UpdateViewportCamera() {
    camera.viewportSize = Vec2{ ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };
    
    // get coordinates of the top-left corner of the window to know the absolute coordinates to draw
    camera.canvas_p0 = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };
    camera.offset = Vec2{ camera.viewportSize.x / 2 , camera.viewportSize.y / 2 };

	// if the window is hovered and we are dragging, update the world space offset to move the camera
    
    if(ImGui::IsWindowHovered()){
        // PANNING
        if (iState.draggedNodeId == -1 && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            camera.displacement.x -= ImGui::GetIO().MouseDelta.x / camera.zoom;
            camera.displacement.y -= ImGui::GetIO().MouseDelta.y / camera.zoom;
	    }

        // ZOOM
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            // Optional: Zoom towards mouse position
            camera.zoom *= (1 + wheel * 0.15f);
            if (camera.zoom < 0.1f) camera.zoom = 0.1f; // Cap zoom
			else if (camera.zoom > 10.0f) camera.zoom = 10.0f;
        }
    }

}

void MyApp::HandleInput(){

    // GET MOUSE POSITION AND SET HOVERING STATE
    ImVec2 mousePos = ImGui::GetMousePos();
    iState.worldMousePos = camera.ScreenToWorld(mousePos);


    if (ImGui::IsWindowHovered()) {

        iState.hoveredNodeId = simulation.GetHoveredNodeId(iState.worldMousePos);
        iState.hoveredLink = simulation.GetHoveredLink(iState.worldMousePos);

        if (isCreationMode == 1) {
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
        else if (isCreationMode == 0) {
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
        // MANIPULATE INFO
        else if (isCreationMode == 2) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (iState.hoveredNodeId != -1) {
                    // display info of node in the right panel
                    infoPanelNode = iState.hoveredNodeId;
                    infoPanelLink = Link{};

                }
                else if (iState.hoveredLink.nodeA != -1){
                    infoPanelNode = -1;
                    infoPanelLink = iState.hoveredLink;
                }
                else {
                    infoPanelNode = -1;
                    infoPanelLink = Link{};

                }

            }
        }
    }

}

void MyApp::ParameterWindowUI(){

    ImGui::Begin("Parameters window");
    ImGui::PushItemWidth(100.0f);

    static int numNodesRGx = 5;
	static int numNodesRGy = 5;

    if (ImGui::Button("Load graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.readGraphML();
    }
    if (ImGui::Button("Save graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.saveGraphML();
    }

    if (ImGui::Button("Create regular graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.createGridGraph(numNodesRGx, numNodesRGy, camera.viewportSize.x, camera.viewportSize.y);
    }
    ImGui::DragInt("Width (RG)", &numNodesRGx, 0.1f, 1, 40);
    ImGui::DragInt("Height (RG)", &numNodesRGy, 0.1f, 1, 40);

    static int numNodesER = 20;
    static float pER = 0.05f;

    if (ImGui::Button("Create Erdos-Renyi graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.createRandomGraph(numNodesER, pER, camera.viewportSize.x, camera.viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (ER)", &numNodesER, 0.25f, 1, 100);
    ImGui::DragFloat("Edge probability (ER)", &pER, 0.0025f, 0.0f, 1.0f, "%.2f");

    static int numNodesBA = 20;
    static int k = 1;

    if (ImGui::Button("Create Barabasi-Albert graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        simulation.createBarabasiAlbertGraph(numNodesBA, k, camera.viewportSize.x, camera.viewportSize.y);
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

    ImGui::DragFloat("Repulsion force", &simulation.repulsionForce, 10000.0f, 0.0f, 20000000.0f, "%.0f");
    ImGui::DragFloat("Spring Length", &simulation.springLength, 0.5f, 0.0f, 500.0f, "%.0f");
    ImGui::DragFloat("Spring Stiffness", &simulation.springStiffness, 1.0f, 0.0f, 2500.0f, "%.0f");
    ImGui::DragFloat("Damping", &simulation.damping, 0.001f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Central gravity", &simulation.centralGravity, 2.0f, 0.0f, 3000.0f, "%.0f");

    ImGui::PopItemWidth();

    if (!isPhysicsPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
        if (ImGui::Button("PLAY PHYSICS", ImVec2(ImGui::GetContentRegionAvail().x, 20)))
            isPhysicsPlaying = true;
        ImGui::PopStyleColor();
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.6f, 0.0f, 1.0f));
        if (ImGui::Button("PAUSE PHYSICS", ImVec2(ImGui::GetContentRegionAvail().x, 20)))
            isPhysicsPlaying = false;
        ImGui::PopStyleColor();
    }

    // leave some space
	ImGui::Dummy(ImVec2(0, 10));

    if (!isSimulationPlaying) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
        if (ImGui::Button("PLAY SIMULATION", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
            isSimulationPlaying = true;
        ImGui::PopStyleColor();
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.6f, 0.0f, 1.0f));
        if (ImGui::Button("PAUSE SIMULATION", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
            isSimulationPlaying = false;
        ImGui::PopStyleColor();
    }

    ImGui::RadioButton("Epidemics", (int32_t*) &simType, (int)SIM_SIR); ImGui::SameLine();
    ImGui::RadioButton("Kuramoto", (int32_t*) &simType, (int)SIM_KURAMOTO);

    if (simType == SIM_SIR) {
        ImGui::RadioButton("SIR", &simulation.epidemicType, 0); ImGui::SameLine();
        ImGui::RadioButton("SIS", &simulation.epidemicType, 1);

        ImGui::PushItemWidth(100.0f);
        ImGui::DragFloat("Infection rate", &simulation.lambdaInfection, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Recovery rate", &simulation.muRecovery, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::PopItemWidth();

        if (ImGui::Button("Recover all nodes", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
            simulation.RecoverAll();
        }

        // Switch between creating network and infecting nodes
        ImGui::RadioButton("Create network", &isCreationMode, 0); ImGui::SameLine();
        ImGui::RadioButton("Network data", &isCreationMode, 2);
        ImGui::RadioButton("Infect nodes", &isCreationMode, 1);

        drawInfectedPlot();

    }
    if (simType == SIM_KURAMOTO) {
        ImGui::PushItemWidth(100.0f);
        ImGui::DragFloat("Coupling strength", &simulation.couplingStrength, 0.01f, 0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Frequency multiplier", &simulation.globalFrequency, 0.01f, 0.0f, 5.0f, "%.2f");
        ImGui::PopItemWidth();

        // Switch between creating network and infecting nodes
        ImGui::RadioButton("Create network", &isCreationMode, 0); ImGui::SameLine();
        ImGui::RadioButton("Network data", &isCreationMode, 2);
        ImGui::RadioButton("Reset phases", &isCreationMode, 1);

        if (ImGui::Button("Randomize all phases", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
            simulation.RandomPhases();
        }
        if (ImGui::Button("Synchronize all phases", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
            simulation.SynchronizePhases();
        }

    }

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
