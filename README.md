# SIRgraphs

An interactive graph simulation of the SIR (Susceptible, Infected, Recovered) and Kuramoto model, built from scratch in C++ using Dear ImGui. 

This project allows you to build complex networks in real-time, save or load graphs, and simulate how an infection spreads based on parameters you can tune (implemented Kuramoto model to see how networks synchronize).

<img width="485" alt="Captura de pantalla 2026-04-04 213206" src="https://github.com/user-attachments/assets/18a041f7-8fd6-453e-8aa3-fc87efec76d0" />
<img width="485" alt="Captura de pantalla 2026-04-04 213335" src="https://github.com/user-attachments/assets/3551bed6-ee3f-4a65-85eb-965aa6e92dc9" />

You can dock the viewport and settings window on the background window however you like.

## User Guide

### Loading and saving a graph

The application supports very simple graphs (undirected, unweighted) in graphml format. You can load a .graphml file by pressing "Load graph" at the top of the parameter window. Graphml files can be created using libraries like Python's ```networkx```. You can try loading the "Karate club graph" by executing:
```
import networkx as nx
graph = nx.karate_club_graph()
nx.write_graphml(graph, "karate.graphml")
```
You can build your own graph and save it into graphml format, that can be read using said libraries and tools like Gephi (to be tested).

### Building a graph

While the **Create network** button all the way below the parameter window is selected, **left-clicking** anywhere in the viewport creates a node. While hovering over a node, **right-clicking** will delete it. **Left-clicking and dragging** from one node to another will create a link between them, and right-clicking over a link will delete it.

On the **parameters window**, there are options to easily create a random graph (**Erdos-Renyi**) and a scale-free graph (**Barabási-Albert**) from the specified number of nodes and edges. You can also add nodes one by one following their respective graph rules and based on the edge probability or links per node options above. Pressing **shift** while **left clicking** "Add Erdos-Renyi node" or "Add Barabasi-Albert node" will add 10 nodes.

You can also press Delete graph to start again.

Below you can find physical parameters to tweak the way the nodes are arranged and move.

### Simulating an epidemic

Once your graph is created, select the "Infect nodes" option below. Left-click a node to infect it, or right-click a node to recover it. Press the "PLAY SIMULATION" button to start the dynamics.

You can change the infection and recovery rates, and switch between the Susceptible-Infected-Recovered (SIR) model and the Susceptible-Infected-Susceptible (SIS) model.

### New stuff

New simulation -> **Kuramoto model** for synchornization. Switch between Kuramoto and SIR models with a button.

New graph -> **regular network**. Build a 2d grid with set width and height.

## DIRECT DOWNLOAD

The compiled executables can be found in the [Releases Window](../../releases/latest) for Windows, MacOS and Linux . The application may not be signed with a creator certificate, so the operative system will flag them as dangerous. If you don't want to compile the code from source, press "execute anyways" on Windows. On macOS, unzip the file, right-click the application and select "Open", then open anyways. On linux, unzip the file and execute it from the command line.


## Building from Source (CMake)

This project uses Git Submodules for its dependencies. You must clone the repository recursively to download Dear ImGui alongside the source code.

```bash
# 1. Clone the repository with submodules
git clone --recursive https://github.com/PabloValgaRuiz/SIRgraphs.git SIRgraphs
cd SIRgraphs
```
### On Windows (Visual Studio 2022)

Visual Studio has native support for CMake. Open Visual Studio 2022.

Select Open a local folder and select the cloned SIRgraphs directory.

In the top toolbar, select SIRgraphs.exe as the startup item and run.

### On Windows (CMake)

If you use different compiler tools for C++, you can compile the application using CMake.

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
.\Release\SIRgraphs.exe
```

### On macOS / Linux (CMake)

Make sure you have CMake and GLFW installed on your system

On macOS
```
brew install cmake glfw
```
On Debian/Ubuntu
```
sudo apt install cmake libglfw3-dev
```
Then
```
# Create a build directory
mkdir build
cd build

# Configure and compile
cmake ..
make

# Run the program
./SIRgraphs
```


## Dependencies

Dear ImGui (Docking Branch) - Easy UI library.

GLFW - Window and OpenGL context management.

PCG Random - High-performance random number generation.

nativefiledialog-extended - Opening file explorer and selecting files

PugiXML - XML file support for saving and loading GraphML files
