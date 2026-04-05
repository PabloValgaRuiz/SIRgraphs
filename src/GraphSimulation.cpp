#include "GraphSimulation.h"
#include "vendor/PugiXML/src/pugixml.hpp"
#include <vendor/nativefiledialog-extended/src/include/nfd.h>

void GraphSimulation::UpdatePhysics(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    for (int idx = 0; idx < accel.size(); idx++) {
        accel[idx].x = 0;
        accel[idx].y = 0;
    }
    addForces();

    // VERLET INTEGRATION

    for (int idx = 0; idx < position.size(); idx++) {
        Vec2 temp_position = position[idx];

        position[idx].x += ((position[idx].x - last_position[idx].x) + accel[idx].x * deltaTime * deltaTime) * (1 - damping);
        position[idx].y += ((position[idx].y - last_position[idx].y) + accel[idx].y * deltaTime * deltaTime) * (1 - damping);
        last_position[idx] = temp_position;
    }


}

void GraphSimulation::addForces() {

    // node repulsion
    for (int nodeA = 0; nodeA < position.size(); nodeA++) {
        for (int nodeB = 0; nodeB < position.size(); nodeB++) {
            if (nodeA > nodeB) continue; // Don't do this twice for every pair

            Vec2 distanceVec = Vec2{position[nodeA].x - position[nodeB].x, position[nodeA].y - position[nodeB].y};
            float distanceSquared = distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y;

            if (distanceSquared > 600 * 600) continue;

            float minSafeDistanceSq = 20.0f * 20.0f;
            distanceSquared = std::max(distanceSquared, minSafeDistanceSq);

            Vec2 repulsionStrength;
            repulsionStrength.x = 2 * repulsionForce * distanceVec.x / (distanceSquared);
            repulsionStrength.y = 2 * repulsionForce * distanceVec.y / (distanceSquared);

            accel[nodeA].x += repulsionStrength.x;
            accel[nodeA].y += repulsionStrength.y;

            accel[nodeB].x -= repulsionStrength.x;
            accel[nodeB].y -= repulsionStrength.y;
        }
    }

    // link attraction like springs
    for (const auto& link : links) {
        int nodeA = link.nodeA;
        int nodeB = link.nodeB;

        Vec2 distanceVec = Vec2{position[nodeA].x - position[nodeB].x, position[nodeA].y - position[nodeB].y};
        float distance = sqrt(distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y);
        Vec2 normalizedDistanceVec = Vec2{distanceVec.x / (distance + 0.01f), distanceVec.y / (distance + 0.01f)};
        Vec2 attractionStrength;
        attractionStrength.x = springStiffness * (springLength - distance) * normalizedDistanceVec.x;
        attractionStrength.y = springStiffness * (springLength - distance) * normalizedDistanceVec.y;

        accel[nodeA].x += attractionStrength.x;
        accel[nodeA].y += attractionStrength.y;

        accel[nodeB].x -= attractionStrength.x;
        accel[nodeB].y -= attractionStrength.y;
    }

    for (int node = 0; node < position.size(); node++) {

        // Slight force towards the center of the window
        // get center of the world space
        Vec2 center = Vec2{0,0};
        accel[node].x += (center.x - position[node].x) * centralGravity;
        accel[node].y += (center.y - position[node].y) * centralGravity;

    }

}


void GraphSimulation::UpdateSIR(float deltaTime)
{
    static std::uniform_real_distribution<double> dist(0, 1);

    newStateSIR = stateSIR;

    // Infection
    for (const auto& link : links) {
        int nodeA = link.nodeA;
        int nodeB = link.nodeB;

        if (stateSIR[nodeA] == I && stateSIR[nodeB] == S) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newStateSIR[nodeB] = I;
            }
        }
        else if (stateSIR[nodeA] == S && stateSIR[nodeB] == I) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newStateSIR[nodeA] = I;
            }
        }
    }
    //Recovery
    for (int node = 0; node < stateSIR.size(); node++){
        if (stateSIR[node] == I) {
            if (dist(rng) < muRecovery * deltaTime) {
                if (epidemicType == 0)
                    newStateSIR[node] = R;
                else if (epidemicType == 1)
                    newStateSIR[node] = S;
            }
        }
    }
    std::swap(stateSIR, newStateSIR);
}

void GraphSimulation::updateKuramoto(float deltaTime) {
    newPhaseKur = phaseKur;

    for (const auto& link : links) {
        float phaseDiff = phaseKur[link.nodeB] - phaseKur[link.nodeA];
        float sinphase = sinf(phaseDiff);
        newPhaseKur[link.nodeA] += couplingStrength * sinphase * deltaTime;
        newPhaseKur[link.nodeB] -= couplingStrength * sinphase * deltaTime;
    }
    
    for (int i = 0; i < phaseKur.size(); i++) {
        newPhaseKur[i] += globalFrequency * frequency[i] * deltaTime;
        newPhaseKur[i] = fmodf(newPhaseKur[i], 2 * PI);
    }
    std::swap(phaseKur, newPhaseKur);

}

void GraphSimulation::createRandomGraph(int numNodes, float p, float width, float height) {

    static std::uniform_real_distribution<float> position_dist(-300, 300);
    static std::uniform_real_distribution<float> p_dist(0, 1);

    deleteGraph();

    for (int i = 0; i < numNodes; i++) {
        addNode(Vec2{ position_dist(rng), position_dist(rng) });
    }
    for (int id1 = 0; id1 < position.size(); id1++){
        for (int id2 = 0; id2 < position.size(); id2++){
            if (p_dist(rng) < p && id1 < id2) {
                addLink(id1, id2);
            }
        }
    }
}

void GraphSimulation::addNodeErdosRenyi(float p) {
    static std::uniform_real_distribution<double> p_dist(0, 1);
    if (position.size() == 0) {
        int id = addNode(Vec2{ 0, 0 });
        return;
    }

    int id = addNode(Vec2{ 0, 0 });

    std::vector<int> connected_nodes;

    for (int key = 0; key < position.size(); key++) {
        if (p_dist(rng) < p && id != key) {
            addLink(id, key);
            connected_nodes.push_back(key);
        }
    }

    static std::uniform_real_distribution<float> rand_pos_shift(-50, 50);
    // get coordinates average and place the node there
    if (connected_nodes.size() > 0) {
        Vec2 avg_coords{ 0,0 };
        for (auto key : connected_nodes) {
            avg_coords.x += position[key].x;
            avg_coords.y += position[key].y;
        }
        // shift the coordinates a bit

        avg_coords = Vec2{ rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
                            rand_pos_shift(rng) + avg_coords.y / connected_nodes.size() };
        position[id] = avg_coords;
    }
    else {
        position[id] = Vec2{ rand_pos_shift(rng), rand_pos_shift(rng) };
    }
}

void GraphSimulation::addNodeBarabasiAlbert(int k) {

    static std::uniform_real_distribution<float> p_dist(0, 1);

    if (position.size() == 0) {
        int id = addNode(Vec2{ 0,0 });
        return;
    }

    if (k > position.size()) k = (int)position.size();

    int id = addNode(Vec2{ 0,0 });

    std::vector<int> degrees; degrees.resize(position.size());

    for (const auto& link : links) {
        degrees[link.nodeA]++;
        degrees[link.nodeB]++;
    }

    auto degrees_copy = degrees;
    degrees.push_back(0);

    std::vector<int> connected_nodes{}; connected_nodes.reserve(k);
    for (int m = 0; m < k; m++) {

        float random_value = p_dist(rng);
        float counter = 0;

        int nLinks = 0;
        for (const auto& degree : degrees_copy) {
            nLinks += degree;
        }

        // node to connect to
        for (int j = 0; j < degrees_copy.size(); j++) {

            if (id == j) continue;

            if (nLinks < 2)
                counter = 1.0;
            else
                counter += (float)degrees_copy[j] / nLinks;

            if (random_value < counter) {
                addLink(id, j);
                degrees[id]++;
                degrees[j]++;

                connected_nodes.push_back(j); // save them for coordinates

                degrees_copy[j] = 0;
                break;
            }
        }
    }
    // get coordinates average and place the node there
    Vec2 avg_coords{ 0,0 };
    for (auto key : connected_nodes) {
        avg_coords.x += position[key].x;
        avg_coords.y += position[key].y;
    }
    // shift the coordinates a bit
    static std::uniform_real_distribution<float> rand_pos_shift(-50, 50);

    avg_coords = Vec2{ rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
        rand_pos_shift(rng) + avg_coords.y / connected_nodes.size() };
    position[id] = avg_coords;
}

void GraphSimulation::createGridGraph(int numNodesX, int numNodesY, float width, float height)
{
    deleteGraph();
    Vec2 gridSize = Vec2{ 60.0f * numNodesX, 60.0f * numNodesY };
	std::vector<int> nodeIds; nodeIds.reserve(numNodesX * numNodesY);
    for(size_t i = 0; i < numNodesX; i++){
        for(size_t j = 0; j < numNodesY; j++){
			nodeIds.push_back(addNode(Vec2{ (i - numNodesX / 2.0f) * gridSize.x / numNodesX, (j - numNodesY / 2.0f) * gridSize.y / numNodesY }));
            if(i > 0){
	            addLink(nodeIds[(i - 1) * numNodesY + j], nodeIds[i * numNodesY + j]);
            }
            if(j > 0){
                addLink(nodeIds[i * numNodesY + j - 1], nodeIds[i * numNodesY + j]);
            }
        }
    }
}


void GraphSimulation::createBarabasiAlbertGraph(int numNodes, int k, float width, float height) {

    static std::uniform_real_distribution<float> position_dist(-300, 300);
    static std::uniform_real_distribution<float> p_dist(0, 1);

    deleteGraph();

    if (k > numNodes) k = numNodes;

    // keep up with degree count

    for (int i = 0; i < k; i++) {
        int id = addNode(Vec2{ position_dist(rng), position_dist(rng) });
        for (int key = 0; key < position.size(); key++) {
            if (id != key) {
                addLink(id, key);
            }
        }
    }


    for (int i = k; i < numNodes; i++) {
        addNodeBarabasiAlbert(k);
    }
}

void GraphSimulation::readGraphML()
{
    deleteGraph();
    std::string path;

    NFD_Init();

    nfdu8char_t* outPath;
    nfdu8filteritem_t filters[1] = { { "GraphML file", "graphml" } };
    nfdopendialogu8args_t args = { 0 };
    args.filterList = filters;
    args.filterCount = 1;
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        puts("Success!");
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
    {
        puts("User pressed cancel.");
    }
    else
    {
        printf("Error: %s\n", NFD_GetError());
    }

    NFD_Quit();


    pugi::xml_document doc;
    if (!doc.load_file(path.c_str())) std::cout << "Failed to read graph" << std::endl;
    else {
        std::unordered_map<std::string, int> ids;
        std::uniform_real_distribution<float> dist;

        bool isPositionData = false, isWeightData = false;
        std::string xkey, ykey, weightkey;

        // Navigate to the graph element
        auto graphml = doc.child("graphml");
        pugi::xml_node graph = graphml.child("graph");

        auto xkeynode = graphml.find_child_by_attribute("key", "attr.name", "x");
        auto ykeynode = graphml.find_child_by_attribute("key", "attr.name", "y");
        auto weightkeynode = graphml.find_child_by_attribute("key", "attr.name", "weight");
        if (xkeynode && ykeynode) {
            isPositionData = true;
            xkey = xkeynode.attribute("id").value();
            ykey = ykeynode.attribute("id").value();
        }
        if (weightkeynode) {
            isWeightData = true;
            weightkey = weightkeynode.attribute("id").value();
        }
        for (pugi::xml_node node : graph.children("node")) {
            int i = addNode(Vec2{ dist(rng), dist(rng) });
            ids.emplace(node.attribute("id").value(), i);

            if (isPositionData) {
                auto x = node.find_child_by_attribute("data", "key", xkey.c_str());
                auto y = node.find_child_by_attribute("data", "key", ykey.c_str());
                position[i] = Vec2{ x.text().as_float(), y.text().as_float() };
            }

        }

        for (pugi::xml_node edge : graph.children("edge")) {
            Link link;
            int nodeA = ids.at(edge.attribute("source").value());
            int nodeB = ids.at(edge.attribute("target").value());
            
            if(isWeightData){
                float weight = edge.find_child_by_attribute("data", "key", weightkey.c_str()).text().as_float();
                addLink(nodeA, nodeB, weight);
            }
            else{
                addLink(nodeA, nodeB);
            }

        }
        std::cout << "Read " << getN() << " nodes and " << getLinks().size() << " edges." << "\n";
        for (auto link : links) {
            std::cout << link.nodeA << ", " << link.nodeB << ": " << link.weight << "\n";
        }
        std::cout << "Success saving graph" << std::endl;
    }
}

void GraphSimulation::saveGraphML()
{
    std::string path;

    NFD_Init();

    nfdu8char_t* outPath;
    nfdu8filteritem_t filters[1] = { { "GraphML file", "graphml" } };
    nfdsavedialogu8args_t args = { 0 };
    args.filterList = filters;
    args.filterCount = 1;
    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        puts("Success!");
        puts(outPath);
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
    {
        puts("User pressed cancel.");
    }
    else
    {
        printf("Error: %s\n", NFD_GetError());
    }

    NFD_Quit();

    pugi::xml_document doc;

    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";

    auto graphml = doc.append_child("graphml");
    graphml.append_attribute("xmlns").set_value("http://graphml.graphdrawing.org/xmlns");

    // POSITION X AND Y
    auto keyX = graphml.append_child("key");
    keyX.append_attribute("id") = "d0";
    keyX.append_attribute("for") = "node";
    keyX.append_attribute("attr.name") = "x";
    keyX.append_attribute("attr.type") = "float";

    auto keyY = graphml.append_child("key");
    keyY.append_attribute("id") = "d1";
    keyY.append_attribute("for") = "node";
    keyY.append_attribute("attr.name") = "y";
    keyY.append_attribute("attr.type") = "float";


    auto graph = graphml.append_child("graph");
    graph.append_attribute("id").set_value("graph");
    graph.append_attribute("edgedefault").set_value("undirected");

    for (int i = 0; i < position.size(); i++) {
        auto node = graph.append_child("node");
        node.append_attribute("id").set_value("n"+std::to_string(i));

        auto dataX = node.append_child("data");
        dataX.append_attribute("key") = "d0";
        dataX.append_child(pugi::node_pcdata).set_value(std::to_string(position[i].x).c_str());

        auto dataY = node.append_child("data");
        dataY.append_attribute("key") = "d1";
        dataY.append_child(pugi::node_pcdata).set_value(std::to_string(position[i].y).c_str());

    }
    for (const auto& link : links) {
        auto edge = graph.append_child("edge");
        edge.append_attribute("source").set_value("n" + std::to_string(link.nodeA));
        edge.append_attribute("target").set_value("n" + std::to_string(link.nodeB));
    }

    if (!doc.save_file(path.c_str())){
        std::cout << "Failed to save graph" << std::endl;
    }
    else{
        std::cout << "Success saving graph" << std::endl;
    }
}

// see if the mouse is over a node
int GraphSimulation::GetHoveredNodeId(Vec2 localMousePos) {
    for (int node = 0; node < position.size(); node++) {

        float dx = position[node].x - localMousePos.x;
        float dy = position[node].y - localMousePos.y;
        float distanceSquared = (dx * dx) + (dy * dy);

        if (distanceSquared <= (radius[node] * radius[node])) {
            return node;
        }
    }
    return -1; // -1 means no node was hovered
}

Link GraphSimulation::GetHoveredLink(Vec2 localMousePos)
{
    // get rectangle from the link and see if the mouse is in it

    float width = 4.0f;// (*2/2); //half the width each side, but also twice so it's easier to click (since the line is thin)
    for (const auto& link : links) {


        Vec2 positionA = position[link.nodeA];
        Vec2 positionB = position[link.nodeB];

        Vec2 vectorAB = Vec2{positionB.x - positionA.x, positionB.y - positionA.y};
        Vec2 vectorAC = Vec2{localMousePos.x - positionA.x, localMousePos.y - positionA.y};

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