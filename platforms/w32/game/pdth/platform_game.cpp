#include "lua_functions.h"
#include "subhook.h"

#include "tweaker/xmltweaker.h"
#include "tweaker/wrenloader.h"

#include <thread>

static std::thread::id main_thread_id;
static subhook::Hook gameUpdateDetour, newStateDetour, luaCloseDetour, node_from_xmlDetour;

static void init_idstring_pointers()
{
	char *tmp;

	tmp = (char*)try_open_property_match_resolver;
	tmp += 0x46;
	blt::platform::last_loaded_name = *((blt::idstring**)tmp);

	tmp = (char*)try_open_property_match_resolver;
	tmp += 0x2A;
	blt::platform::last_loaded_ext = *((blt::idstring**)tmp);
}

static int __fastcall luaL_newstate_new(void* thislol, int edx, char no, char freakin, int clue)
{
	subhook::ScopedHookRemove scoped_remove(&newStateDetour);

	int ret = luaL_newstate(thislol, no, freakin, clue);

	lua_State* L = (lua_State*)*((void**)thislol);
	printf("Lua State: %p\n", (void*)L);
	if (!L) return ret;

	blt::lua_functions::initiate_lua(L);

	return ret;
}

void* __fastcall do_game_update_new(void* thislol, int edx, int* a, int* b)
{
	subhook::ScopedHookRemove scoped_remove(&gameUpdateDetour);

	// If someone has a better way of doing this, I'd like to know about it.
	// I could save the this pointer?
	// I'll check if it's even different at all later.
	if (std::this_thread::get_id() != main_thread_id)
	{
		return do_game_update(thislol, a, b);
	}

	lua_State* L = (lua_State*)*((void**)thislol);

	blt::lua_functions::update(L);

	return do_game_update(thislol, a, b);
}

void lua_close_new(lua_State* L)
{
	subhook::ScopedHookRemove scoped_remove(&luaCloseDetour);

	blt::lua_functions::close(L);
	lua_close(L);
}

static void setup_platform_game()
{
	main_thread_id = std::this_thread::get_id();

	gameUpdateDetour.Install(do_game_update, do_game_update_new);
	newStateDetour.Install(luaL_newstate, luaL_newstate_new);
	luaCloseDetour.Install(lua_close, lua_close_new);

	init_idstring_pointers();

	auto lock = pd2hook::wren::lock_wren_vm();
	WrenVM* vm = pd2hook::wren::get_wren_vm();
}