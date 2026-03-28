cmake_minimum_required(VERSION 3.14)
project(SIRgraphs)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

# Gather all the ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    # ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(APP_SOURCES
    src/main.cpp
    src/Application.cpp
    src/GraphSimulation.cpp
)

# build an executable with ImGui's code and the app's code
add_executable(SIRgraphs MACOSX_BUNDLE ${APP_SOURCES} ${IMGUI_SOURCES})


target_include_directories(SIRgraphs PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pcg
)


# PLATFORM SPECIFIC SETUP

if (WIN32)
    # Use the pre-compiled GLFW binaries within ImGui
    target_include_directories(SIRgraphs PRIVATE
        ${IMGUI_DIR}/examples/libs/glfw/include
    )
    
    target_link_libraries(SIRgraphs PRIVATE
        ${IMGUI_DIR}/examples/libs/glfw/lib-vc2010-64/glfw3.lib
        opengl32
    )

elseif (APPLE)
    # Rely on Homebrew installations
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    
    target_link_libraries(SIRgraphs PRIVATE
        glfw
        OpenGL::GL
    )
    
    # Apple requires these
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(SIRgraphs PRIVATE ${COCOA_FRAMEWORK} ${IOKIT_FRAMEWORK} ${COREVIDEO_FRAMEWORK})

elseif (UNIX AND NOT APPLE)

    find_package(OpenGL REQUIRED)

    include(FetchContent)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    target_link_libraries(SIRgraphs PRIVATE glfw OpenGL::GL)
endif()