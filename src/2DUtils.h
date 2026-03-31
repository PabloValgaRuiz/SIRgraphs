#pragma once

#include "imgui.h"
#include <array>
#include <iostream>

struct Vec2 {
    float x;
    float y;

    Vec2 operator+(const Vec2& vector) {
        return Vec2{ x + vector.x, y + vector.y };
    }
    Vec2 operator-(const Vec2& vector) {
        return Vec2{ x - vector.x, y - vector.y };
    }
    float operator*(const Vec2& vector) {
        return x * vector.x + y * vector.y;
    }
    Vec2 operator*(float k) {
        return Vec2{ x * k, y * k };
    }
    Vec2 operator/(float k) {
        return Vec2{ x / k, y / k };
    }
};

struct Camera2D {
    Vec2 offset{ 0, 0 };
	Vec2 displacement{ 0, 0 }; // Used for panning with mouse drag
    float zoom = 1.0f;
    Vec2 viewportSize{ 800, 800 };
    Vec2 canvas_p0{ 0, 0 }; // Used for ImGui absolute positioning

    // Convert ImGui Screen pixels to Physics World coordinates
    Vec2 ScreenToWorld(ImVec2 screenCoord) const {
        return Vec2{
            ((screenCoord.x - canvas_p0.x - offset.x) / zoom) - displacement.x ,
            ((screenCoord.y - canvas_p0.y - offset.y) / zoom) - displacement.y
        };
    }

    // Convert Physics World coordinates to ImGui Screen pixels
    ImVec2 WorldToScreen(Vec2 worldCoord) const {
        return ImVec2(
            (worldCoord.x + displacement.x) * zoom + canvas_p0.x + offset.x,
            (worldCoord.y + displacement.y) * zoom + canvas_p0.y + offset.y
        );
    }

    std::array<float, 16> GetOrthoMatrix() const {
        // Calculate the world-space bounds of the screen using the inverted formula
        float L = (-offset.x / zoom) - displacement.x;
        float R = ((viewportSize.x - offset.x) / zoom) - displacement.x;

        // We swap Top and Bottom here to flip the Y-axis so 
        // Y grows downwards, matching ImGui and standard 2D canvases.
        float T = (-offset.y / zoom) - displacement.y;
        float B = ((viewportSize.y - offset.y) / zoom) - displacement.y;

        std::array<float, 16> ortho = { 0 }; // Initialize to 0

        // Column 0
        ortho[0] = 2.0f / (R - L);
        // Column 1
        ortho[5] = 2.0f / (T - B);
        // Column 2
        ortho[10] = -1.0f;
        // Column 3 (Translation)
        ortho[12] = -(R + L) / (R - L);
        ortho[13] = -(T + B) / (T - B);
        ortho[15] = 1.0f;

        return ortho;
    }
};

