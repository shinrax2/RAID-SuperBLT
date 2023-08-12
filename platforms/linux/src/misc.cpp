#include "platform.h"

void blt::platform::GetPlatformInformation(lua_State * L)
{
	lua_pushstring(L, "gnu+linux");
	lua_setfield(L, -2, "platform");

	lua_pushstring(L, "x86-64");
	lua_setfield(L, -2, "arch");
}

