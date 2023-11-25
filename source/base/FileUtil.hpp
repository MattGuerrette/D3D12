////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Matt Guerrette 2023.
// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

class FileUtil
{
public:
	static std::string PathForResource(const std::string& resourceName);

	static void* ReadAllBytes(const std::string& resourceName, uint32_t* numBytes);
};
