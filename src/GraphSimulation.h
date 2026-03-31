#pragma once

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <random>
#include "pcg_random.hpp"

#include "2DUtils.h"

enum NodeState {
    S = 0,
    I = 1,
    R = 2
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

struct InteractionState {
    Vec2 worldMousePos{};

    int hoveredNodeId = -1;
    int draggedNodeId = -1;
    Link hoveredLink{};
};


class GraphSimulation{
public:
    void addForces();
    void UpdatePhysics(float deltaTime);
    void UpdateSIR(float deltaTime);

    void addNodeErdosRenyi(float p);
    void addNodeBarabasiAlbert(int k);
	void createGridGraph(int numNodesX, int numNodesY, float width, float height);
    void createRandomGraph(int numNodes, float p, float width, float height);
    void createBarabasiAlbertGraph(int numNodes, int k, float width, float height);

    int GetHoveredNodeId(Vec2 localMousePos);
    Link GetHoveredLink(Vec2 localMousePos);

    // add node

    void addLink(int nodeA, int nodeB) {        
        links.emplace(nodeA, nodeB);
    }

    int addNode(Vec2 pos, Vec2 accel = Vec2{ 0,0 }, float r = 20.0f, NodeState state = S) {
  
        this->position.push_back(pos);
        this->last_position.push_back(pos);
        this->accel.push_back(accel);
        this->radius.push_back(r);
        this->state.push_back(state);

        return (int)position.size() - 1;

    }
    void removeLink(int nodeA, int nodeB) {
        links.erase(Link(nodeA, nodeB));
    }

    void removeNode(int id) {

        int last_id = (int)position.size() - 1;
        std::vector<Link> update_links;
        
        for (auto it = links.begin(); it != links.end(); ) {
            
            // Remove every link the node has
            if (it->nodeA == id || it->nodeB == id) {
                it = links.erase(it);
            }
            
            // If it's not the last node, swap it with the last node. Every link of the last node has to change the stored id
            else if(id != last_id){
                if(it->nodeA == last_id){
                    update_links.emplace_back(id, it->nodeB);
                    it = links.erase(it);
                }
                else if (it->nodeB == last_id) {
                    update_links.emplace_back(it->nodeA, id);
                    it = links.erase(it);
                }
                else
                    ++it;
            }
            else {
                ++it;
            }
        }
        // Add all the updated links
        for(const auto& link : update_links){
            links.emplace(link);
        }

        // Swap the nodes if it's not the last one
        if (id != last_id){
            position[id] = position[last_id];
            last_position[id] = last_position[last_id];
            accel[id] = accel[last_id];
            radius[id] = radius[last_id];
            state[id] = state[last_id];
        }
        // shrink the arrays
        this->position.pop_back();
        this->last_position.pop_back();
        this->accel.pop_back();
        this->radius.pop_back();
        this->state.pop_back();

    }

    void deleteGraph(){
        position.clear();
        last_position.clear();
        accel.clear();
        radius.clear();
        state.clear();
        links.clear();
    }

    void RecoverAll(){
        for (int node = 0; node < state.size(); node++){
	        state[node] = S;
        }
    }

    size_t getN() const {
        return position.size();
	}

    NodeState getNodeState(int nodeId) const {
        return state[nodeId];
	}

    void SetNodeState(int nodeId, NodeState newState) {
        state[nodeId] = newState;
	}

    const std::vector<Vec2>& getNodePositions() const{
        return position;
	}
    const std::vector<float>& getNodeRadius() const{
        return radius;
    }
    const std::unordered_set<Link>& getLinks() const{
		return links;
    }

public:
    // Physics constants
    float repulsionForce = 2000000.0f;
    float springLength = 70.0f;
    float springStiffness = 500.0f;
    float damping = 0.95f;
    float centralGravity = 1000.0f;

    // SIR dynamics
    int epidemicType = 0; // 0 -> SIR, 1 -> SIS
    float lambdaInfection = 0.6f; // Infection rate
    float muRecovery = 0.2f; // Recovery rate



private:

    // random number generator shared between all functions
    pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };

    // Node data
    std::vector<Vec2> position;
    std::vector<Vec2> last_position;
    std::vector<Vec2> accel;
    std::vector<float> radius;
    std::vector<NodeState> state;
    std::vector<NodeState> newState;

    std::unordered_set<Link> links;
};