////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "Keyboard.hpp"

bool Keyboard::IsKeyClicked(const SDL_Scancode key)
{
    return CurrentKeyState_[key] && !PreviousKeyState_[key];
}

bool Keyboard::IsKeyPressed(const SDL_Scancode key)
{
    return CurrentKeyState_[key];
}

void Keyboard::RegisterKeyEvent(const SDL_KeyboardEvent* event)
{
    CurrentKeyState_[event->scancode] = event->down;
}

void Keyboard::Update()
{
    PreviousKeyState_ = CurrentKeyState_;
}
