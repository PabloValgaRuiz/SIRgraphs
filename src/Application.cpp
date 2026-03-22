#include "Application.h"
#include <random>
#include "pcg_random.hpp"




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
    ImGui::Begin("Simulation Viewport", nullptr);

    viewportSize = ImGui::GetContentRegionAvail();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // get coordinates of the top-left corner of the window to know the absolute coordinates to draw
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();

    // GET MOUSE POSITION AND SET HOVERING STATE
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 localMousePos = ImVec2(mousePos.x - canvas_p0.x, mousePos.y - canvas_p0.y);
    int hoveredNodeId = -1;
    Link hoveredLink{};

    if (ImGui::IsWindowHovered()) {
        hoveredNodeId = GetHoveredNodeId(localMousePos);
        hoveredLink = GetHoveredLink(localMousePos);

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
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float clickX = mousePos.x - canvas_p0.x;
                    float clickY = mousePos.y - canvas_p0.y;
                    addNode(ImVec2(clickX, clickY));
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

        ImVec2 absolutePosA = ImVec2(canvas_p0.x + nodeA.position.x, canvas_p0.y + nodeA.position.y);
        ImVec2 absolutePosB = ImVec2(canvas_p0.x + nodeB.position.x, canvas_p0.y + nodeB.position.y);

        ImVec4 color = ImVec4(1.0, 1.0, 1.0, 1.0);
        if (hoveredLink == link) color = ImVec4(1.0, 0.0, 0.0, 1.0);
        draw_list->AddLine(absolutePosA, absolutePosB, ImGui::GetColorU32(color), 4.0f);
    }
    for (const auto& node : nodes) {
        ImVec2 absolutePos = ImVec2(canvas_p0.x + node.second.position.x, canvas_p0.y + node.second.position.y);

        ImVec4 color = setNodeColor(node.second, hoveredNodeId);
        draw_list->AddCircleFilled(absolutePos, node.second.radius, ImGui::GetColorU32(color));

    }
    // DRAW THE LINE BEING DRAGGED FROM NODE TO MOUSE
    if (draggedNodeId != -1) {
        auto draggedPosition = nodes.at(draggedNodeId).position;
        auto absoluteDraggedPosition = ImVec2(canvas_p0.x + draggedPosition.x, canvas_p0.y + draggedPosition.y);
        draw_list->AddLine(absoluteDraggedPosition, mousePos, ImGui::GetColorU32(ImVec4(1.0, 1.0, 1.0, 1.0)), 5.0f);
    }

    //ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 300.0f);
    //ImGui::Text("Node rendered: 1");

    ImGui::End();

    ImGui::Begin("Parameters window");


    ImGui::PushItemWidth(100.0f);

    static int numNodesER = 10;
    static float pER = 0.1f;

    if (ImGui::Button("Create Erdos-Renyi graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        createRandomGraph(numNodesER, pER, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (ER)", &numNodesER, 0.25f, 1, 50);
    ImGui::DragFloat("Edge probability (ER)", &pER, 0.0025f, 0.0f, 1.0f, "%.2f");

    static int numNodesBA = 10;
    static int k = 4;

    if (ImGui::Button("Create Barabasi-Albert graph", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
        createBarabasiAlbertGraph(numNodesBA, k, viewportSize.x, viewportSize.y);
    }
    ImGui::DragInt("Number of nodes (BA)", &numNodesBA, 0.125f, 1, 50);
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

    ImGui::DragFloat("Repulsion force", &repulsionForce, 20000.0f, 20000.0f, 20000000.0f, "%.0f");
    ImGui::DragFloat("Spring Length", &springLength, 0.5f, 5.0f, 500.0f, "%.0f");
    ImGui::DragFloat("Spring Stiffness", &springStiffness, 1.0f, 25.0f, 2500.0f, "%.0f");
    ImGui::DragFloat("Damping", &damping, 0.001f, 0.0f, 1.0f, "%.2f");
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
        // get center of the window
        ImVec2 center = ImVec2(viewportSize.x / 2, viewportSize.y / 2);
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
    // random generator
    static pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
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
                newNodes.at(node.id).state = R;
            }
        }
    }

    nodes = newNodes;



}

void MyApp::createRandomGraph(int numNodes, float p, float width, float height) {

    static pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
    static std::uniform_real_distribution<double> position_dist(0.2, 0.8);
    static std::uniform_real_distribution<double> p_dist(0, 1);

    nodes.clear();
    links.clear();
    for (int i = 0; i < numNodes; i++) {
        addNode(ImVec2(position_dist(rng) * width, position_dist(rng) * height));
    }
    for (int i = 0; i < numNodes; i++) {
        for (int j = i + 1; j < numNodes; j++) {
            if (p_dist(rng) < p) {
                addLink(i, j);
            }
        }
    }
}

void MyApp::addNodeErdosRenyi(float p) {
    static pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
    static std::uniform_real_distribution<double> p_dist(0, 1);
    if (nodes.size() == 0) {
        int id = addNode(ImVec2(0, 0));
        return;
    }

    int id = addNode(ImVec2(0, 0));

    std::vector<int> connected_nodes;

    for (const auto& [key, node] : nodes) {
        if (p_dist(rng) < p) {
            addLink(id, key);
            connected_nodes.push_back(key);
        }
    }

    // get coordinates average and place the node there
    if (connected_nodes.size() > 0) {
        ImVec2 avg_coords(0, 0);
        for (auto key : connected_nodes) {
            avg_coords.x += nodes.at(key).position.x;
            avg_coords.y += nodes.at(key).position.y;
        }
        // shift the coordinates a bit
        static std::uniform_real_distribution<double> rand_pos_shift(0.5, 1.5);

        avg_coords = ImVec2(rand_pos_shift(rng) * avg_coords.x / connected_nodes.size(),
            rand_pos_shift(rng) * avg_coords.y / connected_nodes.size());
        nodes.at(id).position = avg_coords;
    }
}

void MyApp::addNodeBarabasiAlbert(int k) {

    static pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
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
            if (nLinks == 0)
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
    static std::uniform_real_distribution<double> rand_pos_shift(0.8, 1.2);

    avg_coords = ImVec2(rand_pos_shift(rng) * avg_coords.x / connected_nodes.size(),
        rand_pos_shift(rng) * avg_coords.y / connected_nodes.size());
    nodes.at(id).position = avg_coords;
}

void MyApp::createBarabasiAlbertGraph(int numNodes, int k, float width, float height) {
    static pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
    static std::uniform_real_distribution<double> position_dist(0.2, 0.8);
    static std::uniform_real_distribution<double> p_dist(0, 1);

    nodes.clear();
    links.clear();

    if (k > numNodes) k = numNodes;

    // keep up with degree count

    for (int i = 0; i < k; i++) {
        int id = addNode(ImVec2(position_dist(rng) * width, position_dist(rng) * height));
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
