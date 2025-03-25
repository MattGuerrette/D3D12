////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#include "File.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>

File::File(const wchar_t* path)
{
    HANDLE hFile =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Failed to open file");
    }

    DWORD fileSize = GetFileSize(hFile, nullptr);
    if (fileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        throw std::runtime_error("Failed to get file size");
    }

    m_data.resize(fileSize);

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, m_data.data(), fileSize, &bytesRead, nullptr))
    {
        CloseHandle(hFile);
        throw std::runtime_error("Failed to read file");
    }

    CloseHandle(hFile);
}

std::vector<uint8_t> File::Data() const
{
    return m_data;
}
