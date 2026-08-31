#pragma once

#include "config/config_storage.hpp"
#include "config.pb.h"

class ConfigLoader
{
public:
	static bool apply(const ConfigImage &image, ConsoleMode mode);
};
