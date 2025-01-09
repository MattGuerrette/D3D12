////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Camera.hpp"

Camera::Camera(
    const Vector3 position, const float fov, const float aspectRatio, const float nearPlane, const float farPlane)
    : m_position(position), m_direction(Vector3::Forward), m_fieldOfView(fov), m_aspectRatio(aspectRatio),
      m_nearPlane(nearPlane), m_farPlane(farPlane)
{
    UpdateUniforms();
}

void Camera::MoveForward(float dt)
{
    m_position += Direction() * dt * m_speed;
    UpdateUniforms();
}

void Camera::MoveBackward(float dt)
{
    m_position -= Direction() * dt * m_speed;
    UpdateUniforms();
}

void Camera::StrafeLeft(float dt)
{
    m_position += Right() * dt * m_speed;
    UpdateUniforms();
}

void Camera::StrafeRight(float dt)
{
    m_position -= Right() * dt * m_speed;
    UpdateUniforms();
}

Vector3 Camera::Direction() const
{
    return Vector3::Transform(m_direction, m_orientation);
}

Vector3 Camera::Right() const
{
    return Vector3::Transform(-Vector3::Right, m_orientation);
}

Vector3 Camera::Up() const
{
    return Vector3::Transform(Vector3::Up, m_orientation);
}

void Camera::RotateY(const float dt)
{
    const Quaternion rotation = Quaternion::CreateFromYawPitchRoll(dt, 0, 0);
    m_direction = Vector3::Transform(Vector3::Forward, rotation);
    UpdateUniforms();
}

Matrix Camera::ViewProjection() const
{
    return Matrix::CreateLookAt(m_position, m_position + Direction(), Up()) *
           Matrix::CreatePerspectiveFieldOfView(m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::UpdateUniforms()
{
    m_orientation = Quaternion::LookRotation(Direction(), Vector3::Up);
}
