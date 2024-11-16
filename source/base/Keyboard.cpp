
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024. Matt Guerrette. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "Keyboard.hpp"

bool Keyboard::IsKeyClicked(SDL_Scancode key)
{
    return CurrentKeyState_[key] && !PreviousKeyState_[key];
}

bool Keyboard::IsKeyPressed(SDL_Scancode key)
{
    return CurrentKeyState_[key];
}

void Keyboard::RegisterKeyEvent(SDL_KeyboardEvent* event)
{
    CurrentKeyState_[event->scancode] = event->down;
}

void Keyboard::Update()
{
    PreviousKeyState_ = CurrentKeyState_;
}
