#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <random>
#include "pcg_random.hpp"

enum NodeState {
    S = 0,
    I = 1,
    R = 2
};

struct Vec2 {
    float x;
    float y;
};

class Node {
public:
    int id;

    Vec2 position;
    Vec2 last_position;
    Vec2 accel;
    float radius;

    NodeState state;

    Node(int id, Vec2 pos, Vec2 accel, float r, NodeState state = S)
        : id(id), position(pos), last_position(pos), accel(accel), radius(r), state(state) {
    }
};

class Link {
public:
    int nodeA;
    int nodeB;

    Link() {
        nodeA = -1;
        nodeB = -1;
    }

    Link(int a, int b) {
        // Enforce order for undirected graph (smaller ID first).
        nodeA = std::min(a, b);
        nodeB = std::max(a, b);
    }
    bool operator==(const Link& other) const {
        return nodeA == other.nodeA && nodeB == other.nodeB;
    }
};

namespace std {
    template <>
    struct hash<Link> {
        size_t operator()(const Link& l) const {
            // standard way to combine two hashes.
            // hash nodeA, hash nodeB, shift the bits of B, and XOR them together.
            return hash<int>()(l.nodeA) ^ (hash<int>()(l.nodeB) << 1);
        }
    };
}


class UndirectedGraph {
    std::unordered_map<int, Node> nodes;
    std::unordered_set<Link> links;
};

class MyApp {
public:
    MyApp();
    void Render();
    void addForces();
    void UpdatePhysics(float deltaTime);
    void UpdateSIR(float deltaTime);

    void addNodeErdosRenyi(float p);
    void addNodeBarabasiAlbert(int k);
    void createRandomGraph(int numNodes, float p, float width, float height);
    void createBarabasiAlbertGraph(int numNodes, int k, float width, float height);


    // add node

    void addLink(int nodeA, int nodeB) {
        // Only add the link if both nodes actually exist in the map
        if (nodes.find(nodeA) != nodes.end() && nodes.find(nodeB) != nodes.end()) {
            links.emplace(nodeA, nodeB);
        }
    }
    int addNode(Vec2 pos, Vec2 accel = Vec2{0,0}, float r = 20.0f, NodeState state = S) {
        int id = nodes.size();
        while (nodes.find(id) != nodes.end()) {
            id++;
        }
        nodes.emplace(id, Node(id, pos, accel, r, state));
        return id;
    }
    void removeLink(int nodeA, int nodeB) {
        links.erase(Link(nodeA, nodeB));
    }

    void removeNode(int id) {
        nodes.erase(id);
        for (auto it = links.begin(); it != links.end(); ) {
            if (it->nodeA == id || it->nodeB == id) {
                it = links.erase(it);
            }
            else {
                ++it;
            }
        }
    }

private:
    int GetHoveredNodeId(Vec2 localMousePos);
    Link GetHoveredLink(Vec2 localMousePos);
    int draggedNodeId = -1;

    Vec2 viewportSize = Vec2{800.0f, 600.0f};
    float worldSpaceZoom = 1.0f;
    Vec2 worldSpaceOffset = Vec2{400.0f, 300.0f};

    std::unordered_map<int, Node> nodes;
    std::unordered_set<Link> links;

    // Switch between creating network and infecting nodes
    int isInfectionMode = 0;

    // Physics constants
    float repulsionForce = 2000000.0f;
    float springLength = 70.0f;
    float springStiffness = 500.0f;
    float damping = 0.95f;
    float centralGravity = 1000.0f;

    // SIR dynamics

    // random number generator shared between all functions
    pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };

    bool isSimulationPlaying = false;
    float simulationTime = 0.0f;

    int epidemicType = 0; // 0 -> SIR, 1 -> SIS

    float lambdaInfection = 0.6f; // Infection rate
    float muRecovery = 0.2f; // Recovery rate



    // LOG NODES AND LINKS
    void logNodesLinks() {
        std::cout << "\033[F\33[2K"; // Move up and clear line 2
        std::cout << "\033[F\33[2K"; // Move up and clear line 1
        std::cout << "\r";           // Reset cursor to start of line
        std::cout << "Nodes: " << nodes.size() << std::endl;
        std::cout << "Links: " << links.size() << std::endl;
    }
};
