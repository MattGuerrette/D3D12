////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "FileUtil.hpp"

#include <SDL.h>

std::string FileUtil::PathForResource(const std::string& resourceName)
{
    return resourceName;
}

void* FileUtil::ReadAllBytes(const std::string& resourceName, uint32_t* numBytes)
{
    SDL_RWops* ops = SDL_RWFromFile(resourceName.c_str(), "rb");

    size_t numBytesRead;
    void* result = SDL_LoadFile_RW(ops, &numBytesRead, SDL_FALSE);
    if (result == NULL)
    {
        return nullptr;
    }

    *numBytes = numBytesRead;

    return result;
}