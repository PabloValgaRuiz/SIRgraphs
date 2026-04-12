cmake_minimum_required(VERSION 3.16.3)
project(SIRgraphs LANGUAGES C CXX)

if (APPLE)
    enable_language(OBJC)
endif()

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw)

# Gather all the ImGui source files (don't need docs, tests and examples')
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    #${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad)

set (GLAD_SOURCES
    ${GLAD_DIR}/src/glad.c
)

set(APP_SOURCES
    src/main.cpp
    src/Application.cpp
    src/GraphSimulation.cpp
    src/Renderer.cpp
)

# COMPILE EXTERNAL LIBRARIES

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/PugiXML)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/nativefiledialog-extended)


# build an executable with ImGui's code and the app's code
add_executable(SIRgraphs MACOSX_BUNDLE ${APP_SOURCES} ${IMGUI_SOURCES} ${GLAD_SOURCES})

target_link_libraries(SIRgraphs PRIVATE
                pugixml-static
                nfd
                glfw
                )

target_include_directories(SIRgraphs PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLAD_DIR}/include
    ${GLFW_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pcg
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/PugiXML/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/nativefiledialog-extended/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/
)


# PLATFORM SPECIFIC SETUP



if (APPLE)

    # PLIST to let macos know the graphml extension
    set_target_properties(SIRgraphs PROPERTIES 
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")

    # Rely on macos opengl installation
    find_package(OpenGL REQUIRED)

    # Apple requires these
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    # and these so that nativefiledialog-extended works
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(UTI_FRAMEWORK UniformTypeIdentifiers)

    

    target_link_libraries(SIRgraphs PRIVATE
        OpenGL::GL

        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}

        ${APPKIT_FRAMEWORK} 
        ${UTI_FRAMEWORK}
    )

elseif (UNIX AND NOT APPLE)

    find_package(OpenGL REQUIRED)
    target_link_libraries(SIRgraphs PRIVATE OpenGL::GL)

endif()
