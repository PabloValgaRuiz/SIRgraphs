#include "GraphSimulation.h"


void GraphSimulation::UpdatePhysics(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    for (auto& pair : nodes) {
        Node& node = pair.second;
        node.accel.x = 0;
        node.accel.y = 0;
    }
    addForces();

    // VERLET INTEGRATION

    for (auto& pair : nodes) {
        Node& node = pair.second;
        Vec2 temp_position = node.position;

        node.position.x += ((node.position.x - node.last_position.x) + node.accel.x * deltaTime * deltaTime) * (1 - damping);
        node.position.y += ((node.position.y - node.last_position.y) + node.accel.y * deltaTime * deltaTime) * (1 - damping);
        node.last_position = temp_position;
    }


}

void GraphSimulation::addForces() {

    // node repulsion
    for (auto& pairA : nodes) {
        for (auto& pairB : nodes) {
            if (pairA.first == pairB.first) continue; // Don't repel yourself

            Node& nodeA = pairA.second;
            Node& nodeB = pairB.second;

            Vec2 distanceVec = Vec2{nodeA.position.x - nodeB.position.x, nodeA.position.y - nodeB.position.y};
            float distanceSquared = distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y;

            if (distanceSquared > 600 * 600) continue;

            float minSafeDistanceSq = 20.0f * 20.0f;
            distanceSquared = std::max(distanceSquared, minSafeDistanceSq);

            Vec2 repulsionStrength;
            repulsionStrength.x = repulsionForce * distanceVec.x / (distanceSquared);
            repulsionStrength.y = repulsionForce * distanceVec.y / (distanceSquared);

            nodeA.accel.x += repulsionStrength.x;
            nodeA.accel.y += repulsionStrength.y;

            nodeB.accel.x -= repulsionStrength.x;
            nodeB.accel.y -= repulsionStrength.y;
        }
    }

    // link attraction like springs
    for (const auto& link : links) {
        Node& nodeA = nodes.at(link.nodeA);
        Node& nodeB = nodes.at(link.nodeB);

        Vec2 distanceVec = Vec2{nodeA.position.x - nodeB.position.x, nodeA.position.y - nodeB.position.y};
        float distance = sqrt(distanceVec.x * distanceVec.x + distanceVec.y * distanceVec.y);
        Vec2 normalizedDistanceVec = Vec2{distanceVec.x / (distance + 0.01f), distanceVec.y / (distance + 0.01f)};
        Vec2 attractionStrength;
        attractionStrength.x = springStiffness * (springLength - distance) * normalizedDistanceVec.x;
        attractionStrength.y = springStiffness * (springLength - distance) * normalizedDistanceVec.y;

        nodeA.accel.x += attractionStrength.x;
        nodeA.accel.y += attractionStrength.y;

        nodeB.accel.x -= attractionStrength.x;
        nodeB.accel.y -= attractionStrength.y;
    }

    for (auto& pair : nodes) {
        Node& node = pair.second;

        // Slight force towards the center of the window
        // get center of the world space
        Vec2 center = Vec2{0,0};
        node.accel.x += (center.x - node.position.x) * centralGravity;
        node.accel.y += (center.y - node.position.y) * centralGravity;

    }

}


void GraphSimulation::UpdateSIR(float deltaTime)
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
                if (epidemicType == 0)
                    newNodes.at(node.id).state = R;
                else if (epidemicType == 1)
                    newNodes.at(node.id).state = S;
            }
        }
    }
    nodes = newNodes;
}

void GraphSimulation::createRandomGraph(int numNodes, float p, float width, float height) {

    static std::uniform_real_distribution<float> position_dist(-300, 300);
    static std::uniform_real_distribution<float> p_dist(0, 1);

    nodes.clear();
    links.clear();
    for (int i = 0; i < numNodes; i++) {
        addNode(Vec2{ position_dist(rng), position_dist(rng) });
    }
    for (const auto& [id1, node_1] : nodes) {
        for (const auto& [id2, node_2] : nodes) {
            if (p_dist(rng) < p && id1 < id2) {
                addLink(id1, id2);
            }
        }
    }
}

void GraphSimulation::addNodeErdosRenyi(float p) {
    static std::uniform_real_distribution<double> p_dist(0, 1);
    if (nodes.size() == 0) {
        int id = addNode(Vec2{ 0, 0 });
        return;
    }

    int id = addNode(Vec2{ 0, 0 });

    std::vector<int> connected_nodes;

    for (const auto& [key, node] : nodes) {
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
            avg_coords.x += nodes.at(key).position.x;
            avg_coords.y += nodes.at(key).position.y;
        }
        // shift the coordinates a bit

        avg_coords = Vec2{ rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
                            rand_pos_shift(rng) + avg_coords.y / connected_nodes.size() };
        nodes.at(id).position = avg_coords;
    }
    else {
        nodes.at(id).position = Vec2{ rand_pos_shift(rng), rand_pos_shift(rng) };
    }
}

void GraphSimulation::addNodeBarabasiAlbert(int k) {

    static std::uniform_real_distribution<float> p_dist(0, 1);

    if (nodes.size() == 0) {
        int id = addNode(Vec2{ 0,0 });
        return;
    }

    if (k > nodes.size()) k = nodes.size();

    int id = addNode(Vec2{ 0,0 });

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
    Vec2 avg_coords{ 0,0 };
    for (auto key : connected_nodes) {
        avg_coords.x += nodes.at(key).position.x;
        avg_coords.y += nodes.at(key).position.y;
    }
    // shift the coordinates a bit
    static std::uniform_real_distribution<float> rand_pos_shift(-50, 50);

    avg_coords = Vec2{ rand_pos_shift(rng) + avg_coords.x / connected_nodes.size(),
        rand_pos_shift(rng) + avg_coords.y / connected_nodes.size() };
    nodes.at(id).position = avg_coords;
}

void GraphSimulation::createBarabasiAlbertGraph(int numNodes, int k, float width, float height) {

    static std::uniform_real_distribution<float> position_dist(-300, 300);
    static std::uniform_real_distribution<float> p_dist(0, 1);

    nodes.clear();
    links.clear();

    if (k > numNodes) k = numNodes;

    // keep up with degree count

    for (int i = 0; i < k; i++) {
        int id = addNode(Vec2{ position_dist(rng), position_dist(rng) });
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

// see if the mouse is over a node
int GraphSimulation::GetHoveredNodeId(Vec2 localMousePos) {
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

Link GraphSimulation::GetHoveredLink(Vec2 localMousePos)
{
    // get rectangle from the link and see if the mouse is in it
    for (const auto& link : links) {

        float width = 6.0f;

        Vec2 positionA = nodes.at(link.nodeA).position;
        Vec2 positionB = nodes.at(link.nodeB).position;

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