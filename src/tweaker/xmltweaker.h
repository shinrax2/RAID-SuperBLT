#pragma once

#include "platform.h"

namespace raidhook
{
	namespace tweaker
	{
		char* tweak_raid_xml(char* text, int text_length);
		void free_tweaked_raid_xml(char* text);

		void ignore_file(blt::idfile file);

		extern bool tweaker_enabled;
	}; // namespace tweaker
}; // namespace raidhook
