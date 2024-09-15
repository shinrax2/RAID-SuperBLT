#include "platform.h"

#include "console/console.h"
#include "signatures/signatures.h"
#include "assets/assets.h"
#include <util/util.h>

#include <fstream>
#include <string>

using namespace std;
using namespace pd2hook;

static CConsole* console = NULL;
blt::idstring *blt::platform::last_loaded_name = idstring_none, *blt::platform::last_loaded_ext = idstring_none;

#include <platform_game.cpp>

void blt::platform::InitPlatform()
{
	// Set up logging first, so we can see messages from the signature search process
#ifdef INJECTABLE_BLT
	gbl_mConsole = new CConsole();
#else
	ifstream infile("mods/developer.txt");
	string debug_mode;
	if (infile.good())
	{
		debug_mode = "post"; // default value
		infile >> debug_mode;
	}
	else
	{
		debug_mode = "disabled";
	}

	if (debug_mode != "disabled")
		console = new CConsole();
#endif

	SignatureSearch::Search();

	blt::win32::InitAssets();

	setup_platform_game();
}

void blt::platform::ClosePlatform()
{
	// Okay... let's not do that.
	// I don't want to keep this in memory, but it CRASHES THE SHIT OUT if you delete this after all is said and done.
	if (console) delete console;
}

void blt::platform::GetPlatformInformation(lua_State * L)
{
	lua_pushstring(L, "mswindows");
	lua_setfield(L, -2, "platform");

	#if defined(_M_AMD64)
		lua_pushstring(L, "x86-64");
	#else
		lua_pushstring(L, "x86");
	#endif
	lua_setfield(L, -2, "arch");

	lua_pushstring(L, "raid");
	lua_setfield(L, -2, "game");
}

void blt::platform::win32::OpenConsole()
{
	if (!console)
	{
		console = new CConsole();
	}
}

void * blt::platform::win32::get_lua_func(const char* name)
{
	// Only allow getting the Lua functions
	if (strncmp(name, "lua", 3)) return NULL;

	// Don't allow getting the setup functions
	if (!strncmp(name, "luaL_newstate", 13)) return NULL;

	return SignatureSearch::GetFunctionByName(name);
}

subhook::Hook luaCallDetour;

bool blt::platform::lua::GetForcePCalls()
{
	return luaCallDetour.IsInstalled();
}

void blt::platform::lua::SetForcePCalls(bool state)
{
	// Don't change if already set up
	if (state == GetForcePCalls()) return;

	if (state)
	{
		luaCallDetour.Install(lua_call, blt::lua_functions::perform_lua_pcall);
		PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls will now be forced");
	}
	else
	{
		luaCallDetour.Remove();
		PD2HOOK_LOG_LOG("blt.forcepcalls(): Protected calls are no longer being forced");
	}
}
