#include "util/util.h"
#include "tweaker/db_hooks.h"
#include "dbutil/Datastore.h"

#include "platform.h"
#include "subhook.h"

#include <map>

using pd2hook::tweaker::dbhook::hook_asset_load;

// A wrapper class to store strings in the format that PAYDAY 2 does
// Since Microsoft might (and it seems they have) change their string class, we
// need this.
class PDString
{
  public:
	explicit PDString(std::string str) : s(std::move(str))
	{
		data = s.c_str();
		len = s.length();
		cap = s.length();
	}

  private:
	// The data layout that mirrors PD2's string, must be 24 bytes
	const char* data;
	uint8_t padding[12]{};
	int len;
	int cap;

	// String that can deal with the actual data ownership
	std::string s;
};

static_assert(sizeof(PDString) == 24 + sizeof(std::string), "PDString is the wrong size!");

static std::map<blt::idfile, std::string> custom_assets;

static void open_custom_asset(void* archive, std::string asset)
{
	std::string ds_name;
	PDString pd_name(ds_name);

	BLTFileDataStore* datastore = BLTFileDataStore::Open(asset);

	if (!datastore)
	{
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff) - 1, "Failed to open custom asset '%s' while loading.", asset);
		PD2HOOK_LOG_ERROR(buff);
	}

	Archive_ctor(archive, &pd_name, datastore, 0, datastore->size(), false);
}

// The signature is the same for all try_open methods, so one typedef will work for all of them.
typedef void(__thiscall* try_open_t)(void* this_, void* archive, blt::idstring type, blt::idstring name, int u1, int u2);

static void hook_load(try_open_t orig, subhook::Hook& hook, void* this_, void* archive, blt::idstring type, blt::idstring name, int u1, int u2);

#define DECLARE_PASSTHROUGH(func)                                                                                            \
	static subhook::Hook hook_##func;                                                                                        \
	void __fastcall stub_##func(void* this_, int edx, void* archive, blt::idstring type, blt::idstring name, int u1, int u2) \
	{                                                                                                                        \
		hook_load((try_open_t)func, hook_##func, this_, archive, type, name, u1, u2);                                        \
	}

#define DECLARE_PASSTHROUGH_ARRAY(id)                                                                                      \
	static subhook::Hook hook_##id;                                                                                        \
	void __fastcall stub_##id(void* this_, int edx, void* archive, blt::idstring type, blt::idstring name, int u1, int u2) \
	{                                                                                                                      \
		hook_load((try_open_t)try_open_functions.at(id), hook_##id, this_, archive, type, name, u1, u2);                   \
	}

static void hook_load(try_open_t orig, subhook::Hook& hook, void* this_, void* archive, blt::idstring type, blt::idstring name, int u1, int u2)
{
	// Try hooking this asset, and see if we need to handle it differently
	BLTAbstractDataStore* datastore = nullptr;
	int64_t pos = 0, len = 0;
	std::string ds_name;
	blt::idfile asset_file = blt::idfile(name, type);

	bool found = hook_asset_load(asset_file, &datastore, &pos, &len, ds_name, false);

	// If we do need to load a custom asset as a result of that, do so now
	// That just means making our own version of of the archive
	if (found)
	{
		PDString pd_name(ds_name);
		Archive_ctor(archive, &pd_name, datastore, pos, len, false);
		return;
	}

	if (custom_assets.count(asset_file))
	{
		std::string asset = custom_assets[asset_file];

		open_custom_asset(archive, asset);
		return;
	}

	subhook::ScopedHookRemove scoped_remove(&hook);
	orig(this_, archive, type, name, u1, u2);

	// Read the probably_not_loaded_flag to see if this archive failed to load - if so try again but also
	// look for hooks with the fallback bit set
	bool probably_not_loaded_flag = *(bool*)((char*)archive + 0x30);
	if (probably_not_loaded_flag)
	{
		if (hook_asset_load(asset_file, &datastore, &pos, &len, ds_name, true))
		{
			// Note the deadbeef is for the stack padding argument - see the comment on this signature's declaration
			// for more information.
			PDString pd_name(ds_name);
			Archive_ctor(archive, &pd_name, datastore, pos, len, false);
			return;
		}
	}
}

// PDTH DB:create_entry, DB:remove_entry, DB:has

int create_entry_ex(lua_State* L)
{
	luaL_checktype(L, 2, LUA_TUSERDATA);
	luaL_checktype(L, 3, LUA_TUSERDATA);
	luaL_checktype(L, 4, LUA_TSTRING);

	blt::idstring* extension = (blt::idstring*)lua_touserdata(L, 2);
	blt::idstring* name = (blt::idstring*)lua_touserdata(L, 3);

	blt::idfile asset_file = blt::idfile(*name, *extension);

	size_t len;
	const char* filename_c = luaL_checklstring(L, 4, &len);
	std::string filename = std::string(filename_c, len);

	if (custom_assets.count(asset_file))
	{
		// Appears this is actually allowed in the basegame
		// char buff[1024];
		// snprintf(buff, 1024, "File already exists in replacement DB! %lx.%lx (%s)", name, extension,
		// filename.c_str()); luaL_error(L, buff);
	}

	custom_assets[asset_file] = filename;
	return 0;
}

int create_entry(lua_State* L)
{
	// Chop off anything after the 3rd argument
	if (lua_gettop(L) > 4)
		lua_settop(L, 4);

	return create_entry_ex(L);
}

int remove_entry(lua_State* L)
{
	luaL_checktype(L, 2, LUA_TUSERDATA);
	luaL_checktype(L, 3, LUA_TUSERDATA);

	blt::idstring* extension = (blt::idstring*)lua_touserdata(L, 2);
	blt::idstring* name = (blt::idstring*)lua_touserdata(L, 3);
	blt::idfile asset_file = blt::idfile(*name, *extension);

	custom_assets.erase(asset_file);

	return 0;
}

static subhook::Hook hook_dsl_db_add_members;
static void dt_dsl_db_add_members(lua_State* L)
{
	subhook::ScopedHookRemove scoped_remove(&hook_dsl_db_add_members);

	// Make sure we do ours first, so they get overwritten if the basegame
	// implements them in the future
	lua_pushcclosure(L, create_entry, 0);
	lua_setfield(L, -2, "create_entry");

	lua_pushcclosure(L, remove_entry, 0);
	lua_setfield(L, -2, "remove_entry");

	dsl_db_add_members(L);
}

static subhook::Hook hook_maindb_script_has;
int dt_maindb_script_has(lua_State* L)
{
	subhook::ScopedHookRemove scoped_remove(&hook_maindb_script_has);

	blt::idstring* extension = (blt::idstring*)lua_touserdata(L, 2);
	blt::idstring* name = (blt::idstring*)lua_touserdata(L, 3);

	if (extension != nullptr && name != nullptr) {
		blt::idfile asset_file = blt::idfile(*name, *extension);
		if (custom_assets.count(asset_file))
		{
			lua_pushboolean(L, true);
			return 1;
		}
	}

	return maindb_script_has(L);
}

static void setup_extra_asset_hooks()
{
	hook_dsl_db_add_members.Install(dsl_db_add_members, dt_dsl_db_add_members);
	hook_maindb_script_has.Install(maindb_script_has, dt_maindb_script_has);
}