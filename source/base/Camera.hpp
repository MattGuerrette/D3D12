
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphicsMath.hpp"

class Camera
{
public:
    explicit Camera(Vector3 position, float fov, float aspectRatio, float nearPlane, float farPlane);

    [[nodiscard]] Matrix ViewProjection() const;

    [[nodiscard]] Vector3 Right() const;

    [[nodiscard]] Vector3 Up() const;

    [[nodiscard]] Vector3 Direction() const;

    void MoveForward(float dt);

    void MoveBackward(float dt);

    void StrafeLeft(float dt);

    void StrafeRight(float dt);

    void RotateY(float dt);

private:
    void UpdateUniforms();

    Quaternion m_orientation;
    Vector3    m_position;
    Vector3    m_direction;
    float      m_fieldOfView;
    float      m_aspectRatio;
    float      m_nearPlane;
    float      m_farPlane;
    float      m_speed = 5.0f;
};
