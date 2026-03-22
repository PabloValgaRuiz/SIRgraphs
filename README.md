# SIRgraphs

An interactive graph simulation of the SIR (Susceptible, Infected, Recovered) epidemic model, built from scratch in C++ using Dear ImGui. 

This project allows you to build node networks in real-time and watch how an infection spreads based on parameters you can change.

<img width="1278" height="801" alt="Captura de pantalla 2026-03-22 181840" src="https://github.com/user-attachments/assets/53b0c423-bef0-409e-a9c6-5bf132c384b0" />

You can dock the viewport and settings window on the background window however you like.

## Building from Source (CMake)

This project uses Git Submodules for its dependencies. You must clone the repository recursively to download Dear ImGui alongside the source code.

```bash
# 1. Clone the repository with submodules
git clone --recursive https://github.com/PabloValgaRuiz/SIRgraphs.git
cd SIRgraphs
```
### On windows (Visual Studio 2022)

Visual Studio has native support for CMake.

Open Visual Studio 2022.

Select Open a local folder and select the cloned SIRgraphs directory.

In the top toolbar, select SIRgraphs.exe as the startup item and run.

### macOS / Linux (CMake)

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
