#include "Application.h"

#include <random>




MyApp::MyApp()
{
    ImGui::GetIO().FontGlobalScale = 1.3f;
}

// see if the mouse is over a node
int MyApp::GetHoveredNodeId(ImVec2 localMousePos) {
    for (const auto& pair : nodes) {
        const Node& node = pair.second;

        float dx = node.position.x - localMousePos.x;
        float dy = node.position.y - localMousePos.y;
        float distanceSquared = (dx * dx) + (dy * dy);

        if (distanceSquared <= (node.radius * node.radius)) {
            return node.id;
        }
    }
    return -1; // -1 means no node was hovered
}

Link MyApp::GetHoveredLink(ImVec2 localMousePos)
{
    // get rectangle from the link and see if the mouse is in it
    for (const auto& link : links) {

        float width = 6.0f;

        ImVec2 positionA = nodes.at(link.nodeA).position;
        ImVec2 positionB = nodes.at(link.nodeB).position;

        ImVec2 vectorAB = ImVec2(positionB.x - positionA.x, positionB.y - positionA.y);
        ImVec2 vectorAC = ImVec2(localMousePos.x - positionA.x, localMousePos.y - positionA.y);

        // USE PROJECTIONS TO SEE IF THE MOUSE IS ON THE SEGMENT:
        //CHECK: AB_ort dot AC < 2 * 2 (proof below, also don't need to check >0 cause the width goes both ways)
        float distanceAB2 = vectorAB.x * vectorAB.x + vectorAB.y * vectorAB.y;
        float projectionABortAC2 = vectorAB.y * vectorAC.x - vectorAB.x * vectorAC.y;
        if (projectionABortAC2 * projectionABortAC2 < width * width * distanceAB2) {

            // CHECK: 0 < AB dot AC < AB dot AB (proof: 0 < Proj < LenAB, where Proj = ABunit dot AC -> lenAB * Proj = AB dot AC)
            if (vectorAB.x * vectorAC.x + vectorAB.y * vectorAC.y > 0 && vectorAB.x * vectorAC.x + vectorAB.y * vectorAC.y < distanceAB2) {
                return link;
            }
        }
    }
    return Link{};
}

static ImVec4 setNodeColor(const Node& node, int hoveredNodeId) {

    if (node.id == hoveredNodeId)
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    switch (node.state) {
    case S:
        return ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    case I:
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    case R:
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

static ImVec2 worldToScreenTransform(ImVec2 worldCoord, ImVec2 canvas_p0 = ImVec2(0, 0), float zoom = 1.0f, ImVec2 offset = ImVec2(0, 0)) {
    return ImVec2(  (worldCoord.x * zoom) + canvas_p0.x + offset.x,
                    (worldCoord.y * zoom) + canvas_p0.y + offset.y);
}
static ImVec2 screenToWorldTransform(ImVec2 screenCoord, ImVec2 canvas_p0 = ImVec2(0,0), float zoom = 1.0f, ImVec2 offset = ImVec2(0,0)) {
    return ImVec2(  (screenCoord.x - canvas_p0.x - offset.x) / zoom,
                    (screenCoord.y - canvas_p0.y - offset.y) / zoom);
}

void MyApp::Render() {


    float deltaTime = ImGui::GetIO().DeltaTime;
    if (deltaTime > 0.04f) {
        deltaTime = 0.04f;
    }
    UpdatePhysics(deltaTime * 2);

    if (isSimulationPlaying) {
        UpdateSIR(deltaTime);
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(0, viewport);

    // create a new ImGui window called "Simulation Viewport"
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation Viewport", nullptr);

    viewportSize = ImGui::GetContentRegionAvail();
    worldSpaceOffset = ImVec2(viewportSize.x / 2, viewportSize.y / 2);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // get coordinates of the top-left corner of the window to know the absolute coordinates to draw
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();

    // GET MOUSE POSITION AND SET HOVERING STATE
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 worldMousePos = screenToWorldTransform(mousePos, canvas_p0, worldSpaceZoom, worldSpaceOffset);
    int hoveredNodeId = -1;
    Link hoveredLink{};

    if (ImGui::IsWindowHovered()) {

        // ZOOM

        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            // Optional: Zoom towards mouse position
            worldSpaceZoom += wheel * 0.1f;
            if (worldSpaceZoom < 0.1f) worldSpaceZoom = 0.1f; // Cap zoom
        }

        hoveredNodeId = GetHoveredNodeId(worldMousePos);
        hoveredLink = GetHoveredLink(worldMousePos);

        if (isInfectionMode == 1) {
            // if we are in infection mode, left clicking on a node infects it and right clicking on a node makes it susceptible
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (hoveredNodeId != -1) {
                    nodes.at(hoveredNodeId).state = I;
                }
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (hoveredNodeId != -1) {
                    nodes.at(hoveredNodeId).state = S;
                }
            }
        }
        else if (isInfectionMode == 0) {
            // delete a node where you right click if there is a node there
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (hoveredNodeId != -1) {
                    removeNode(hoveredNodeId);
                }
                else if (hoveredLink.nodeA != -1) {
                    removeLink(hoveredLink.nodeA, hoveredLink.nodeB);
                }
            }

            // LEFT CLICK PRESSED
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // add a link between two nodes when left clicking and dragging from one node to another
                if (hoveredNodeId != -1) {
                    draggedNodeId = hoveredNodeId;
                }
                // add a node where you left click and there isn't already a node there
                else {
                    addNode(ImVec2(worldMousePos.x, worldMousePos.y));
                }
            }
            // LEFT CLICK RELEASED
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                // if we were dragging from a node and we release over another node, add a link between them
                if (draggedNodeId != -1 && hoveredNodeId != -1)
                    if (draggedNodeId != hoveredNodeId)
                        addLink(draggedNodeId, hoveredNodeId);
                draggedNodeId = -1;

            }
        }
    }

    // Draw links and nodes
    for (const auto& link : links) {
        const Node& nodeA = nodes.at(link.nodeA);
        const Node& nodeB = nodes.at(link.nodeB);

        ImVec2 screenPosA = worldToScreenTransform(nodeA.position, canvas_p0, worldSpaceZoom, worldSpaceOffset);
        ImVec2 screenPosB = worldToScreenTransform(nodeB.position, canvas_p0, worldSpaceZoom, worldSpaceOffset);

        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
        if (hoveredLink == link) color = ImVec4(1.0, 0.0, 0.0, 1.0);
        draw_list->AddLine(screenPosA, screenPosB, ImGui::GetColorU32(color), 4.0f * worldSpaceZoom);
    }
    for (const auto& node : nodes) {
        ImVec2 screenPos = worldToScreenTransform(node.second.position, canvas_p0, worldSpaceZoom, worldSpaceOffset);

        ImVec4 color = setNodeColor(node.second, hoveredNodeId);
        draw_list->AddCircleFilled(screenPos, node.second.radius * worldSpaceZoom, ImGui::GetColorU32(color));

    }
    // DRAW THE LINE BEING DRAGGED FROM NODE TO MOUSE
    if (draggedNodeId != -1) {
        auto draggedPosition = nodes.at(draggedNodeId).position;
        auto screenDraggedPosition = worldToScreenTransform(draggedPosition, canvas_p0, worldSpaceZoom, worldSpaceOffset);
        draw_list->AddLine(screenDraggedPosition, mousePos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)), 4.0f * worldSpaceZoom);
    }

    //ImGui::SetCursorPosY();
    char textInfo[64];
    sprintf(textInfo, "Nodes: %i\nLinks: %i", (int)nodes.size(), (int)links.size());
    ImGui::Text(textInfo);

    ImGui::End();

    ImGui::Begin("Parameters window");


    ImGui::PushItemWidth(100.0f);

    static int numNodesER = 20;
    static float pER = 0.05f;

    if (ImGui::Button("Create Erdos-Renyi graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        createRandomGraph(numNodesER, pER, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (ER)", &numNodesER, 0.25f, 1, 100);
    ImGui::DragFloat("Edge probability (ER)", &pER, 0.0025f, 0.0f, 1.0f, "%.2f");

    static int numNodesBA = 20;
    static int k = 1;

    if (ImGui::Button("Create Barabasi-Albert graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        createBarabasiAlbertGraph(numNodesBA, k, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (BA)", &numNodesBA, 0.125f, 1, 100);
    ImGui::DragInt("Links per node (BA)", &k, 0.05f, 1, 10);

    ImGui::PopItemWidth();

    if (ImGui::Button("Add Erdos-Renyi node", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        addNodeErdosRenyi(pER);
    }
    if (ImGui::Button("Add Barabasi-Albert node", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        addNodeBarabasiAlbert(k);
    }

    if (ImGui::Button("Delete graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        nodes.clear();
        links.clear();
    }

    ImGui::PushItemWidth(100.0f);

    ImGui::DragFloat("Repulsion force", &repulsionForce, 10000.0f, 10000.0f, 20000000.0f, "%.0f");
    ImGui::DragFloat("Spring Length", &springLength, 0.5f, 5.0f, 500.0f, "%.0f");
    ImGui::DragFloat("Spring Stiffness", &springStiffness, 1.0f, 25.0f, 2500.0f, "%.0f");
    ImGui::DragFloat("Damping", &damping, 0.001f, 0.5f, 1.0f, "%.2f");
    ImGui::DragFloat("Central gravity", &centralGravity, 2.0f, 1.0f, 3000.0f, "%.0f");

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

    ImGui::RadioButton("SIR", &epidemicType, 0); ImGui::SameLine();
    ImGui::RadioButton("SIS", &epidemicType, 1);

    ImGui::PushItemWidth(100.0f);
    ImGui::DragFloat("Infection rate", &lambdaInfection, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Recovery rate", &muRecovery, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::PopItemWidth();

    // Switch between creating network and infecting nodes
    ImGui::RadioButton("Create network", &isInfectionMode, 0); ImGui::SameLine();
    ImGui::RadioButton("Infect nodes", &isInfectionMode, 1);

    if (ImGui::Button("Recover all nodes", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        for (auto& [key, node] : nodes) {
            node.state = S;
        }
    }


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
        for (const auto& [key, node] : nodes) {
            if (node.state == I) infectedCount += 1;
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
    ImGui::PlotLines("I", values, IM_COUNTOF(values), values_offset , 0, 0.0f, maxValue * 1.2f, ImVec2(0.0f, 200.0f));
    ImGui::PopItemWidth();

    ImGui::End();

    //ImGui::ShowDemoWindow();
}

void MyApp::UpdatePhysics(float deltaTime)
{
    if (deltaTime <= 0.0f) return;

    // node repulsion
    for (auto& pairA : nodes) {
        for (auto& pairB : nodes) {
            if (pairA.first == pairB.first) continue; // Don't repel yourself

            Node& nodeA = pairA.second;
            Node& nodeB = pairB.second;

            ImVec2 distanceVec = ImVec2(nodeA.position.x - nodeB.position.x, nodeA.position.y - nodeB.position.y);
            float distanceSquared = distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y;
            
            if (distanceSquared > 800 * 800) continue;

            ImVec2 repulsionStrength;
            repulsionStrength.x = repulsionForce * distanceVec.x / (distanceSquared + 0.01f);
            repulsionStrength.y = repulsionForce * distanceVec.y / (distanceSquared + 0.01f);

            nodeA.velocity.x += repulsionStrength.x * deltaTime;
            nodeA.velocity.y += repulsionStrength.y * deltaTime;

            nodeB.velocity.x -= repulsionStrength.x * deltaTime;
            nodeB.velocity.y -= repulsionStrength.y * deltaTime;
        }
    }

    // link attraction like springs
    for (const auto& link : links) {
        Node& nodeA = nodes.at(link.nodeA);
        Node& nodeB = nodes.at(link.nodeB);

        ImVec2 distanceVec = ImVec2(nodeA.position.x - nodeB.position.x, nodeA.position.y - nodeB.position.y);
        float distance = sqrt(distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y);
        ImVec2 normalizedDistanceVec = ImVec2(distanceVec.x / (distance + 0.01f), distanceVec.y / (distance + 0.01f));
        ImVec2 attractionStrength;
        attractionStrength.x = springStiffness * (springLength - distance) * normalizedDistanceVec.x;
        attractionStrength.y = springStiffness * (springLength - distance) * normalizedDistanceVec.y;

        nodeA.velocity.x += attractionStrength.x * deltaTime;
        nodeA.velocity.y += attractionStrength.y * deltaTime;

        nodeB.velocity.x -= attractionStrength.x * deltaTime;
        nodeB.velocity.y -= attractionStrength.y * deltaTime;
    }

    // update positions based on velocity and apply damping
    for (auto& pair : nodes) {
        Node& node = pair.second;


        // Slight force towards the center of the window
        // get center of the world space
        ImVec2 center = ImVec2(0, 0);
        node.velocity.x += (center.x - node.position.x) * centralGravity * deltaTime;
        node.velocity.y += (center.y - node.position.y) * centralGravity * deltaTime;

        // damping
        node.velocity.x *= (1 - damping);
        node.velocity.y *= (1 - damping);

        // Update position based on velocity and time step
        node.position.x += node.velocity.x * deltaTime;
        node.position.y += node.velocity.y * deltaTime;
    }

}


void MyApp::UpdateSIR(float deltaTime)
{
    static std::uniform_real_distribution<double> dist(0, 1);

    auto newNodes = nodes;


    // Infection
    for (const auto& link : links) {
        const auto& nodeA = nodes.at(link.nodeA);
        const auto& nodeB = nodes.at(link.nodeB);

        if (nodeA.state == I && nodeB.state == S) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newNodes.at(nodeB.id).state = I;
            }
        }
        else if (nodeA.state == S && nodeB.state == I) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newNodes.at(nodeA.id).state = I;
            }
        }
    }
    //Recovery
    for (const auto& [key, node] : nodes) {
        if (node.state == I) {
            if (dist(rng) < muRecovery * deltaTime) {
                if(epidemicType == 0)
                    newNodes.at(node.id).state = R;
                else if (epidemicType == 1)
                    newNodes.at(node.id).state = S;
            }
        }
    }

    nodes = newNodes;



}

void MyApp::createRandomGraph(int numNodes, float p, float width, float height) {

    static std::uniform_real_distribution<double> position_dist(-300, 300);
    static std::uniform_real_distribution<double> p_dist(0, 1);

    nodes.clear();
    links.clear();
    for (int i = 0; i < numNodes; i++) {
        addNode(ImVec2(position_dist(rng), position_dist(rng)));
    }
    for (const auto& [id1, node_1] : nodes) {
        for (const auto& [id2, node_2] : nodes) {
            if (p_dist(rng) < p && id1 < id2) {
                addLink(id1, id2);
            }
        }
    }
}

void MyApp::addNodeErdosRenyi(float p) {
    static std::uniform_real_distribution<double> p_dist(0, 1);
    if (nodes.size() == 0) {
        int id = addNode(ImVec2(0, 0));
        return;
    }

    int id = addNode(ImVec2(0, 0));

    std::vector<int> connected_nodes;

    for (const auto& [key, node] : nodes) {
        if (p_dist(rng) < p && id != key) {
            addLink(id, key);
            connected_nodes.push_back(key);
        }
    }

    static std::uniform_real_distribution<double> rand_pos_shift(-50, 50);
    // get coordinates average and place the node there
    if (connected_nodes.size() > 0) {
        ImVec2 avg_coords(0, 0);
        for (auto key : connected_nodes) {
            avg_coords.x += nodes.at(key).position.x;
            avg_coords.y += nodes.at(key).position.y;
        }
        // shift the coordinates a bit

        avg_coords = ImVec2(rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
                            rand_pos_shift(rng) + avg_coords.y / connected_nodes.size());
        nodes.at(id).position = avg_coords;
    }
    else {
        nodes.at(id).position = ImVec2(rand_pos_shift(rng), rand_pos_shift(rng));
    }
}

void MyApp::addNodeBarabasiAlbert(int k) {


    static std::uniform_real_distribution<double> p_dist(0, 1);

    if (nodes.size() == 0) {
        int id = addNode(ImVec2(0, 0));
        return;
    }

    if (k > nodes.size()) k = nodes.size();

    int id = addNode(ImVec2(0, 0));

    std::unordered_map<int, int> degrees;
    for (const auto& [key, node] : nodes) {
        degrees.emplace(key, 0);
    }
    for (const auto& link : links) {
        degrees.at(link.nodeA)++;
        degrees.at(link.nodeB)++;
    }

    auto degrees_copy = degrees;
    degrees.emplace(id, 0);

    std::vector<int> connected_nodes{}; connected_nodes.reserve(k);
    for (int m = 0; m < k; m++) {

        float random_value = p_dist(rng);
        float counter = 0;

        int nLinks = 0;
        for (auto& [j, degree] : degrees_copy) {
            nLinks += degree;
        }

        // node to connect to
        for (auto& [key, degree] : degrees_copy) {
            if (id == key) continue;

            if (nLinks < 2)
                counter = 1.0;
            else
                counter += (float)degree / nLinks;

            if (random_value < counter) {
                if (id == key) std::cout << "ERROR " << id << std::endl;
                addLink(id, key);
                degrees[id]++;
                degrees[key]++;

                connected_nodes.push_back(key); // save them for coordinates

                degrees_copy.erase(key);
                break;
            }
        }
    }
    // get coordinates average and place the node there
    ImVec2 avg_coords(0, 0);
    for (auto key : connected_nodes) {
        avg_coords.x += nodes.at(key).position.x;
        avg_coords.y += nodes.at(key).position.y;
    }
    // shift the coordinates a bit
    static std::uniform_real_distribution<double> rand_pos_shift(-50, 50);

    avg_coords = ImVec2(rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
        rand_pos_shift(rng) + avg_coords.y / connected_nodes.size());
    nodes.at(id).position = avg_coords;
}

void MyApp::createBarabasiAlbertGraph(int numNodes, int k, float width, float height) {

    static std::uniform_real_distribution<double> position_dist(-300, 300);
    static std::uniform_real_distribution<double> p_dist(0, 1);

    nodes.clear();
    links.clear();

    if (k > numNodes) k = numNodes;

    // keep up with degree count

    for (int i = 0; i < k; i++) {
        int id = addNode(ImVec2(position_dist(rng), position_dist(rng)));
        for (auto& [key, node] : nodes) {
            if (id != key) {
                addLink(id, key);
            }
        }
    }


    for (int i = k; i < numNodes; i++) {
        addNodeBarabasiAlbert(k);
    }
}
