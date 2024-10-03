#include "lua_functions.h"
#include "subhook.h"

#include "tweaker/xmltweaker.h"

#include <thread>
#include <util/util.h>

static std::thread::id main_thread_id;
static subhook::Hook applicationUpdateDetour, newStateDetour, luaCloseDetour, node_from_xmlDetour;

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

static int luaL_newstate_new(void* _this, char a, char b, int c)
{
	subhook::ScopedHookRemove scoped_remove(&newStateDetour);

	int ret = luaL_newstate_exe(_this, a, b, c);

	lua_State* L = (lua_State*)*((void**)_this);
	printf("Lua State: %p\n", (void*)L);
	if (!L) return ret;

	blt::lua_functions::initiate_lua(L);

	return ret;
}

static void* application_update_new(void* _this, int* a, int* b)
{
	subhook::ScopedHookRemove scoped_remove(&applicationUpdateDetour);

	if (std::this_thread::get_id() == main_thread_id)
	{
		blt::lua_functions::update();
	}

	return application_update(_this, a, b);
}

static void lua_close_new(lua_State* L)
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

	void do_xmlload_invoke(void* node, char* data, int* len);

	static void node_from_xml_new_fastcall(void* node, char* data, int* len)
	{
		subhook::ScopedHookRemove scoped_remove(&node_from_xmlDetour);

		char* modded = raidhook::tweaker::tweak_raid_xml(data, *len);
		int modLen = *len;

		if (modded != data)
		{
			modLen = strlen(modded);
		}

		//edit_node_from_xml_hook(false);
		do_xmlload_invoke(node, modded, &modLen);
		//edit_node_from_xml_hook(true);

		raidhook::tweaker::free_tweaked_raid_xml(modded);
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

	applicationUpdateDetour.Install(application_update, application_update_new, subhook::HookOptions::HookOption64BitOffset);
	newStateDetour.Install(luaL_newstate_exe, luaL_newstate_new, subhook::HookOptions::HookOption64BitOffset);
	luaCloseDetour.Install(lua_close_exe, lua_close_new, subhook::HookOptions::HookOption64BitOffset);

	setup_xml_function_addresses();
	node_from_xmlDetour.Install(node_from_xml, node_from_xml_new, subhook::HookOptions::HookOption64BitOffset);

	init_idstring_pointers();
}