#include "GraphSimulation.h"


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

    newState = state;

    // Infection
    for (const auto& link : links) {
        int nodeA = link.nodeA;
        int nodeB = link.nodeB;

        if (state[nodeA] == I && state[nodeB] == S) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newState[nodeB] = I;
            }
        }
        else if (state[nodeA] == S && state[nodeB] == I) {
            if (dist(rng) < lambdaInfection * deltaTime) {
                newState[nodeA] = I;
            }
        }
    }
    //Recovery
    for (int node = 0; node < state.size(); node++){
        if (state[node] == I) {
            if (dist(rng) < muRecovery * deltaTime) {
                if (epidemicType == 0)
                    newState[node] = R;
                else if (epidemicType == 1)
                    newState[node] = S;
            }
        }
    }
    std::swap(state, newState);
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

    if (k > position.size()) k = position.size();

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
    for (const auto& link : links) {

        float width = 4.0f;

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