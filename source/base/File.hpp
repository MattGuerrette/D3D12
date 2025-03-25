////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023-2025
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

class File
{
public:
    explicit File(const wchar_t* path);
    File(const File& file) = delete;
    File& operator=(const File& file) = delete;

    [[nodiscard]] std::vector<uint8_t> Data() const;

private:
    std::vector<uint8_t> m_data;
};