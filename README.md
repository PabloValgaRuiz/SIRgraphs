# SIRgraphs

An interactive graph simulation of the SIR (Susceptible, Infected, Recovered) epidemic model, built from scratch in C++ using Dear ImGui. 

This project allows you to build node networks in real-time and watch how an infection spreads based on parameters you can change.

<img width="1277" height="798" alt="Captura de pantalla 2026-03-24 093826" src="https://github.com/user-attachments/assets/3241e2c9-69a1-4e46-83c5-da68caba45f6" />

You can dock the viewport and settings window on the background window however you like.

## Simulation

### Building a graph

While the "Create network" below is selected, left-clicking anywhere in the viewport creates a node. While hovering over a node, right-clicking will delete it. Left-clicking and dragging from one node to another will create a link between them, and right-clicking over a link will delete it.

On the parameters window, there are options to easily create a random graph (Erdos-Renyi) and a scale-free graph (Barabási-Albert) from the specified number of nodes and edges. You can also add nodes one by one following their respective graph rules and based on the edge probability or links per node options above.

You can also press Delete graph to start again.

Below you can find physical parameters to tweak the way the nodes are arranged and move.

### Simulating an epidemic

Once your graph is created, select the "Infect nodes" option below. Left-click a node to infect it, or right-click a node to recover it. Press the "PLAY SIMULATION" button to start the dynamics.

You can change the infection and recovery rates, and switch between the Susceptible-Infected-Recovered (SIR) model and the Susceptible-Infected-Susceptible (SIS) model.

### New stuff

New simulation -> Kuramoto model for synchornization. Switch between Kuramoto and SIR models with a button.

New graph -> regular network. Build a 2d grid with set width and height.

Loading and saving graphs

## DIRECT DOWNLOAD

The compiled executables can be found in the [Releases Window](../../releases/latest) for Windows, MacOS and Linux . The application may not be signed with a creator certificate, so the operative system will flag them as dangerous. If you don't want to compile the code from source, press "execute anyways" on Windows. On macOS, unzip the file, right-click the application and select "Open", then open anyways. On linux, unzip the file and execute it from the command line.

If using macOS, make sure glfw is installed.
```
brew install glfw
```

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
