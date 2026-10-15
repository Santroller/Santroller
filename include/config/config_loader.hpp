#pragma once

#include "config/config_storage.hpp"
#include "config.pb.h"

class ConfigLoader
{
public:
	static bool apply(const ConfigImage &image, ConsoleMode mode);
	// Read the pairings from the next config applied, like the first load after boot does
	static void reset_bluetooth_loaded() { s_bluetooth_loaded = false; }

private:
	// Pairings are only read from the first config applied, after that the copy in memory is the real one
	static inline bool s_bluetooth_loaded = false;
};
