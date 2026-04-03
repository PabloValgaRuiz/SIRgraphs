#pragma once

#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <random>
#include "pcg_random.hpp"

#include "2DUtils.h"

static const float PI = 3.14159265f;

enum SimType {
    SIM_SIR = 0,
    SIM_KURAMOTO = 1
};

enum StateSIR {
    S = 0,
    I = 1,
    R = 2
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
    void updateKuramoto(float deltaTime);

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

    int addNode(Vec2 pos, Vec2 accel = Vec2{ 0,0 }, float r = 20.0f, StateSIR stateSIR = S) {
    
        static std::uniform_real_distribution<float> dist(0, 2 * PI);
        static std::uniform_real_distribution<float> freq_dist(0.8f, 1.2f);

        this->position.push_back(pos);
        this->last_position.push_back(pos);
        this->accel.push_back(accel);
        this->radius.push_back(r);
        this->stateSIR.push_back(stateSIR);
        this->phaseKur.push_back(dist(rng));
        this->frequency.push_back(freq_dist(rng));

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
            stateSIR[id] = stateSIR[last_id];
            phaseKur[id] = phaseKur[last_id];
            frequency[id] = frequency[last_id];
        }
        // shrink the arrays
        this->position.pop_back();
        this->last_position.pop_back();
        this->accel.pop_back();
        this->radius.pop_back();
        this->stateSIR.pop_back();
        this->phaseKur.pop_back();
        this->frequency.pop_back();
    }

    void deleteGraph(){
        position.clear();
        last_position.clear();
        accel.clear();
        radius.clear();
        stateSIR.clear();
        phaseKur.clear();
        frequency.clear();
        links.clear();
    }

    void RecoverAll(){
        for (int node = 0; node < stateSIR.size(); node++){
	        stateSIR[node] = S;
        }
    }
    void RandomPhases() {
        std::uniform_real_distribution<float> dist(0, 2*PI);
        for (int node = 0; node < phaseKur.size(); node++) {
            phaseKur[node] = dist(rng);
        }
    }

    size_t getN() const {
        return position.size();
	}

    StateSIR getNodeState(int nodeId) const {
        return stateSIR[nodeId];
	}
    float getNodePhase(int nodeId) const {
        return phaseKur[nodeId];
    }

    void SetNodeState(int nodeId, StateSIR newStateSIR) {
        stateSIR[nodeId] = newStateSIR;
	}
    void SetNodePhase(int nodeId, float newphase) {
        phaseKur[nodeId] = newphase;
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

    // Kuramoto dynamics
    float couplingStrength = 1.0f;
    float globalFrequency = 5.0f;


private:

    // random number generator shared between all functions
    pcg64 rng{ pcg_extras::seed_seq_from<std::random_device>{} };

    // Node data
    std::vector<Vec2> position;
    std::vector<Vec2> last_position;
    std::vector<Vec2> accel;
    std::vector<float> radius;

    std::vector<StateSIR> stateSIR;
    std::vector<StateSIR> newStateSIR;

    std::vector<float> phaseKur;
    std::vector<float> newPhaseKur;
    std::vector<float> frequency;


    std::unordered_set<Link> links;
};