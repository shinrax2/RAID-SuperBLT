#include "lua_functions.h"
#include "subhook.h"

#include "tweaker/xmltweaker.h"

#include <thread>

static std::thread::id main_thread_id;
static subhook::Hook gameUpdateDetour, newStateDetour, luaCloseDetour, node_from_xmlDetour;

static void init_idstring_pointers()
{
	char *tmp;

	tmp = (char*)try_open_property_match_resolver;
	tmp += 0x3A;
	tmp += *(unsigned int*)tmp + 4; // 64-Bit RIP Offset MOV

	blt::platform::last_loaded_name = (blt::idstring*)tmp;

	tmp = (char*)try_open_property_match_resolver;
	tmp += 0x33;
	tmp += *(unsigned int*)tmp + 4; // 64-Bit RIP Offset MOV

	blt::platform::last_loaded_ext = (blt::idstring*)tmp;
}

static int __fastcall luaL_newstate_new(void* thislol, char no, char freakin, int clue)
{
	subhook::ScopedHookRemove scoped_remove(&newStateDetour);

	int ret = luaL_newstate(thislol, no, freakin, clue);

	lua_State* L = (lua_State*)*((void**)thislol);
	printf("Lua State: %p\n", (void*)L);
	if (!L) return ret;

	blt::lua_functions::initiate_lua(L);

	return ret;
}

//void* __fastcall do_game_update_new(void* thislol, int* a, int* b)
//{
//	subhook::ScopedHookRemove scoped_remove(&gameUpdateDetour);
//
//	// If someone has a better way of doing this, I'd like to know about it.
//	// I could save the this pointer?
//	// I'll check if it's even different at all later.
//	if (std::this_thread::get_id() != main_thread_id)
//	{
//		return do_game_update(thislol, a, b);
//	}
//
//	lua_State* L = (lua_State*)*((void**)thislol);
//
//	blt::lua_functions::update(L);
//
//	return do_game_update(thislol, a, b);
//}

void lua_close_new(lua_State* L)
{
	subhook::ScopedHookRemove scoped_remove(&luaCloseDetour);

	blt::lua_functions::close(L);
	lua_close(L);
}

//////////// Start of XML tweaking stuff

extern "C"
{
	static void node_from_xml_new_fastcall(void* node, char* data, int* len);

	void (*NFXNF)(void* node, char* data, int* len);
	node_from_xmlptr NFX;

	void node_from_xml_new();

	void __fastcall do_xmlload_invoke(void* node, char* data, int* len);

	static void node_from_xml_new_fastcall(void* node, char* data, int* len)
	{
		subhook::ScopedHookRemove scoped_remove(&node_from_xmlDetour);

		char* modded = pd2hook::tweaker::tweak_pd2_xml(data, *len);
		int modLen = *len;

		if (modded != data)
		{
			modLen = strlen(modded);
		}

		//edit_node_from_xml_hook(false);
		do_xmlload_invoke(node, modded, &modLen);
		//edit_node_from_xml_hook(true);

		pd2hook::tweaker::free_tweaked_pd2_xml(modded);
	}

	static void setup_xml_function_addresses()
	{
		NFXNF = &node_from_xml_new_fastcall;
		NFX = node_from_xml;
	}
}

static void setup_platform_game()
{
	main_thread_id = std::this_thread::get_id();

	//gameUpdateDetour.Install(do_game_update, do_game_update_new, subhook::HookOptions::HookOption64BitOffset);
	newStateDetour.Install(luaL_newstate, luaL_newstate_new, subhook::HookOptions::HookOption64BitOffset);
	luaCloseDetour.Install(lua_close, lua_close_new, subhook::HookOptions::HookOption64BitOffset);

	setup_xml_function_addresses();
	node_from_xmlDetour.Install(node_from_xml, node_from_xml_new, subhook::HookOptions::HookOption64BitOffset);

	init_idstring_pointers();
}