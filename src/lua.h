#pragma once

#include <stdint.h>

#include "../platforms/w32/signatures/sigdef.h"

inline uint64_t luaX_toidstring(lua_State *L, int index)
{
	return *(uint64_t*) lua_touserdata(L, index);
}
