# SIRgraphs

An interactive graph simulation of the SIR (Susceptible, Infected, Recovered) epidemic model, built from scratch in C++ using Dear ImGui. 

This project allows you to build node networks in real-time and watch how an infection spreads based on parameters you can change.

## Building from Source (Visual Studio 2022)

This project uses Git Submodules for its dependencies. You must clone the repository recursively to download Dear ImGui alongside the source code.

```bash
# 1. Clone the repository with submodules
git clone --recursive https://github.com/PabloValgaRuiz/SIRgraphs.git

# 2. Open the Visual Studio Solution
# Double-click EpidemicSim.sln

# 3. Build and Run
# Set your build configuration to "Release" or "Debug" (x64) and run.
```

## Dependencies

Dear ImGui (Docking Branch) - Easy UI library.

GLFW - Window and OpenGL context management.

PCG Random - High-performance random number generation.
