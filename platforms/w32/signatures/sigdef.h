#pragma once

#ifdef SIG_INCLUDE_MAIN
#include "signatures.h"

#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset, SignatureVR_Both);

#define CREATE_CALLABLE_CLASS_SIGNATURE(vr, name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset, vr);

#define CREATE_SEPARATE_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, signature_vr, mask_vr, offset_vr, ...) \
	typedef retn (*name##ptr)(__VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset, SignatureVR_Desktop); \
	SignatureSearch name ## search_vr(#name, &name, signature_vr, mask_vr, offset_vr, SignatureVR_VR);

#else

// If we're not being included directly from InitiateState.cpp, only declare, not define, variables
#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	extern name ## ptr name;

#define CREATE_CALLABLE_CLASS_SIGNATURE(vr, name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
	extern name ## ptr name;

#define CREATE_SEPARATE_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, signature_vr, mask_vr, offset_vr, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	extern name ## ptr name;

#endif

#if defined(SIG_INCLUDE_MAIN) || defined(SIG_INCLUDE_LJ_INTERNAL)
#define CREATE_LUAJIT_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, __VA_ARGS__)
#else
#define CREATE_LUAJIT_CALLABLE_SIGNATURE(...)
#endif

struct lua_State;

typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);
typedef int(*lua_CFunction) (lua_State *L);
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);
typedef struct luaL_Reg
{
	const char* name;
	lua_CFunction func;
} luaL_Reg;

// From src/luaconf.h
#define LUA_NUMBER		double
#define LUA_INTEGER		ptrdiff_t

// From src/lua.h
// type of numbers in Lua
typedef LUA_NUMBER lua_Number;
typedef LUA_INTEGER lua_Integer;
typedef struct lua_Debug lua_Debug;	// activation record
// Functions to be called by the debuger in specific events
typedef void(*lua_Hook) (lua_State* L, lua_Debug* ar);

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_call, void, "\x48\x63\xC2\x48\x8B\x51\x28\x4C\x8B\xD1\x48\xC1\xE0\x03\x4C\x8B", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pcall, int, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x48\x8B\x59\x10\x41\x8B\xF0\x4C\x63", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_gettop, int, "\x48\x8B\x41\x28\x48\x2B\x41\x20\x48\xC1\xF8\x03\xC3", "xxxxxxxxxxxxx", 0, lua_State*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_settop, void, "\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x85\xD2\x78\x7A\x4C\x8B\x41", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_toboolean, int, "\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8B\x08\x33\xC0\x48\xC1", "xxxxx????xxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tointeger, ptrdiff_t, "\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8B\x10\x48\x8B\xCA\x48\xC1\xF9\x2F\x83\xF9\xF2\x73\x0E", "xxxxx????xxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tonumber, lua_Number, "\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8B\x10\x48\x8B\xCA\x48\xC1\xF9\x2F\x83\xF9\xF2\x77\x09", "xxxxx????xxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_tolstring, const char*, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x49\x8B\xF8\x8B\xDA\x48\x8B\xF1\xE8", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int, size_t*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_objlen, size_t, "\x40\x53\x48\x83\xEC\x20\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x48\x8B", "xxxxxxxxxx????xx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_touserdata, void*, "\x48\x83\xEC\x28\xE8\xCC\xCC\xCC\xCC\x48\x8B\x00", "xxxxx????xxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_loadfilex, int, "\x48\x89\x5C\x24\x20\x55\x56\x57\x48\x81\xEC\x50\x02\x00\x00\x48\x8B", "xxxxxxxxxxxxxxxxx", 0, lua_State*, const char*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_loadstring, int, "\x48\x83\xEC\x48\x48\x89\x54\x24\x30\x48\x83\xC8\xFF\x0F\x1F\x00", "xxxxxxxxxxxxxxxx", 0, lua_State*, const char*)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_loadx, int, "\x4C\x8B\xDC\x53\x56\x57\x48\x00\x00\x00\x00\x00\x00\x48\x8B\x05\x3C\x23\x4A\x00", "xxxxxxx??????xxxxxxx", 0, lua_State*, lua_Reader, void*, const char*, const char*)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_getfield, void, "\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x20\x4D\x8B\xD0\x48\x8B\xD9", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_setfield, void, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x4D\x8B\xD0\x48\x8B\xD9", "xxxxxxxxxxxxxxxx", 0, lua_State*, int, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_createtable, void, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x4C\x8B\x49\x10\x41\x8B\xF8\x8B\xF2", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_newuserdata, void*, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x4C\x8B\x41\x10", "xxxxxxxxxxxxxx", 0, lua_State*, size_t)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_insert, void, "\x4C\x8B\xC9\x85\xD2\x7E\x24\x8D\x42\xFF\x48\x63\xD0\x48\x8B\x41", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_replace, void, "\x40\x53\x48\x83\xEC\x20\x44\x8B\xC2", "xxxxxxxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_remove, void, "\x4C\x8B\xC1\x85\xD2\x7E\x23\x8D\x42\xFF\x48\x63\xD0\x48\x8B\x41\x20\x48\x8B\x49\x28\x48\x8D\x04", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_newstate, lua_State*, "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF2\x48\x8B\xE9\x41\xB9\x70\x0B\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_Alloc, void*)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_close, void, "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\x79\x10\x48\x8B\xB7\xC8\x00\x00\x00\x48", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_gettable, void, "\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\xE8\x00\x00\x00\x00\x4C\x8B\x43\x28\x49\x83\xE8\x08", "xxxxxxxxxx????xxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_settable, void, "\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\xE8\x00\x00\x00\x00\x4C\x8B\x43\x28\x49\x83\xE8\x10\x48\x8B", "xxxxxxxxxx????xxxxxxxxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_setmetatable, int, "\x48\x83\xEC\x28\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x49\x8B\x52\x28\x4C", "xxxxxxxx????xxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_getmetatable, int, "\x48\x83\xEC\x28\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x48\x8B\x08\x48\x8B\xC1\x48\xC1\xF8\x2F\x83\xF8\xF4\x74\x05\x83\xF8\xF3\x75\x13", "xxxxxxxx????xxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushnumber, void, "\x48\x8B\x41\x28\xF2\x0F\x11\x08", "xxxxxxxx", 0, lua_State*, lua_Number)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushinteger, void, "\x48\x8B\x41\x28\x0F\x57\xC0\xF2\x48\x0F\x2A\xC2\xF2\x0F\x11\x00", "xxxxxxxxxxxxxxxx", 0, lua_State*, size_t)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushboolean, void, "\x33\xC0\x85\xD2\x0F\x95\xC0\xFF\xC0\x48\x63\xD0\x48\x8B\x41\x28", "xxxxxxxxxxxxxxxx", 0, lua_State*, bool)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushcclosure, void, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x4C\x8B\x49\x10\x49\x63\xF8\x48\x8B", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, lua_CFunction, int);

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushlstring, void, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x4C\x8B\x49\x10\x49\x8B\xF8\x48\x8B", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, const char*, size_t)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushstring, void, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8B\xFA\x48\x8B\xD9\x48\x85\xD2\x75\x0D\x48\x8B\x41", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, const char*)


CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushfstring, const char*, "\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x48\x83\xEC\x20\x4C\x8B\x41\x10\x48\x8B\xD9", "xxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, const char*, ...)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_checkstack, int, "\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x81\xFA\x40\x1F\x00\x00\x7F", "xxxxxxxxxxxxxxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushvalue, void, "\x48\x83\xEC\x28\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x49\x8B\x52\x28\x48\x8B\x00", "xxxxxxxx????xxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_pushnil, void, "\x48\x8B\x41\x28\x48\xC7\x00\xFF\xFF\xFF\xFF\x48\x83\x41\x28\x08\x48\x8B\x41\x28\x48\x3B\x41\x30\x0F\x83", "xxxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*)

CREATE_NORMAL_CALLABLE_SIGNATURE(luaI_openlib, void, "\x48\x89\x5C\x24\x18\x55\x56\x41\x56\x48\x83\xEC\x20\x48\x8B\x41", "xxxxxxxxxxxxxxxx", 0, lua_State*, const char*, const luaL_Reg*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_ref, int, "\x48\x89\x5C\x24\x20\x57\x48\x83\xEC\x20\x8D\x82\x0F\x27\x00\x00", "xxxxxxxxxxxxxxxx", 0, lua_State*, int);

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawget, void, "\x40\x53\x48\x83\xEC\x20\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x49\x8B\x5A\x28", "xxxxxxxxxx????xxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawset, void, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\x73\x28", "xxxxxxxxxxxxxxxxxxx????xxxx", 0, lua_State*, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawgeti, void, "\x40\x53\x48\x83\xEC\x20\x4D\x63\xD0\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\x08\x48\xBA\xFF\xFF", "xxxxxxxxxxxxx????xxxxxxx", 0, lua_State*, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_rawseti, void, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x4D\x63\xD0\x48\x8B\xD9\xE8", "xxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_type, int, "\x48\x83\xEC\x28\x4C\x8B\xD1\xE8\x00\x00\x00\x00\x48\x8B\x08\x4C", "xxxxxxxx????xxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_typename, const char*, "\x48\x8D\x00\x00\x00\x00\x00\x48\x63\xC2\x48\x8B\x04\xC1\xC3\xCC", "xx?????xxxxxxxxx", 0, lua_State*, int)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_unref, void, "\x45\x85\xC0\x0F\x88\x00\x00\x00\x00\x48\x89\x5C\x24\x08\x48\x89", "xxxxx????xxxxxxx", 0, lua_State*, int, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(lua_equal, int, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x45\x8B\xD0", "xxxxxxxxxxxxx", 0, lua_State*, int, int)

CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_newmetatable, int, "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x41\x56\x48\x83\xEC\x20\x48\x8B\x41\x10", "xxxxxxxxxxxxxxxxxxxx", 0, lua_State*, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_checkudata, int, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x4D\x8B\xD0", "xxxxxxxxxxxxxxxxxx", 0, lua_State*, int, const char*)
CREATE_NORMAL_CALLABLE_SIGNATURE(luaL_error, int, "\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x48\x83\xEC\x20\x4C\x8D\x44\x24\x40", "xxxxxxxxxxxxxxxxxxxxxxxxx", 0, lua_State*, const char*, ...)
CREATE_NORMAL_CALLABLE_SIGNATURE(lua_error, int, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8B\xD9\xE8\xCC\xCC\xCC\xCC\x48\x85\xC0", "xxxxxxxxxxxxxx????xxx", 0, lua_State*)

CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, do_game_update, void*, "\x48\x89\x5C\x24\x10\x57\x48\x83\xEC\x20\x4D\x8B\xD0", "xxxxxxxxxxxxx", 0, int*, int*)
CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, luaL_newstate, int, "\x48\x89\x5C\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xEC\x20\x41\x0F\xB6\xF8\x0F\xB6\xF2\x48\x8B", "xxxxxxxxxxxxxxxxxxxxxxxx", 0, char, char, int)

CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, node_from_xml, void, "\x48\x8B\xC4\x55\x56\x57\x48\x83\xEC\x70\x48\xC7\x40\xA8\xFE\xFF\xFF\xFF", "xxxxxxxxxxxxxxxxxx", 0, void*, char*, int)

#ifdef INCLUDE_TRY_OPEN_FUNCTIONS
extern std::vector<void*> try_open_functions;
#endif
CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, try_open_property_match_resolver, int, "\x48\x89\x54\x24\x10\x55\x53\x56\x57\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\xE9", "xxxxxxxxxxxxxxxxxxxx", 0)

CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, arch_parent, void, "\x40\x55\x56\x57\x41\x56\x41\x57\x48\x8D\x6C\x24\xC9\x48\x81\xEC\xF0\x00\x00\x00", "xxxxxxxxxxxxxxxxxxxx", 0, void* a1, void* a2, void* a3, void* a4);
CREATE_CALLABLE_CLASS_SIGNATURE(SignatureVR_Both, Archive_ctor, void, "\x48\x89\x4C\x24\x08\x57\x48\x83\xEC\x30\x48\xC7\x44\x24\x20\xFE\xFF\xFF\xFF\x48\x89\x5C\x24\x48\x48\x89\x74\x24\x50\x49\x8B\xD9\x49\x8B\xF8\x48\x8B\xF1\x48\xC7\x41\x18\x0F\x00\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0, void* name_stdstr, void* datastore, int64_t pos, int64_t size, bool ukn_prob_compression);

CREATE_NORMAL_CALLABLE_SIGNATURE(index2adr, void*, "\x4C\x8B\xC1\x85\xD2\x7E\x23\x8D\x42\xFF\x48\x63\xD0\x48\x8B\x41\x20\x48\x8D\x04", "xxxxxxxxxxxxxxxxxxxx", 0, lua_State*, int)

// lua c-functions

// From src/lua.h
// Pseudo-indices
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_ENVIRONINDEX	(-10001)
#define LUA_GLOBALSINDEX	(-10002)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))

// From src/lauxlib.h
#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)

// more bloody lua shit
// Thread status; 0 is OK
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5
// From src/lauxlib.h
// Extra error code for 'luaL_load'
#define LUA_ERRFILE     (LUA_ERRERR+1)

// From src/lua.h
// Option for multiple returns in 'lua_pcall' and 'lua_call'
#define LUA_MULTRET	(-1)
#define LUA_TNONE		(-1)
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define lua_pop(L,n)		lua_settop(L, -(n)-1)
#define lua_newtable(L)		lua_createtable(L, 0, 0)
#define lua_isfunction(L,n)	(lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)	(lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)		(lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L, (n)) <= 0)
#define lua_getglobal(L,s)	lua_getfield(L, LUA_GLOBALSINDEX, (s))
#define lua_setglobal(L,s)	lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)

#define luaL_getmetatable(L,n)		(lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_openlib luaI_openlib

// Our own lauxlib functions - see lauxlib.cpp
int luaL_argerror(lua_State *L, int narg, const char *extramsg);
int luaL_checkoption(lua_State *L, int narg, const char *def, const char *const lst[]);
int luaL_typerror(lua_State *L, int narg, const char *tname);
void luaL_checktype(lua_State *L, int narg, int t);
void luaL_checkany(lua_State *L, int narg);
const char *luaL_checklstring(lua_State *L, int narg, size_t *len);
const char *luaL_optlstring(lua_State *L, int narg, const char *def, size_t *len);
lua_Number luaL_checknumber(lua_State *L, int narg);
lua_Number luaL_optnumber(lua_State *L, int narg, lua_Number def);
lua_Integer luaL_checkinteger(lua_State *L, int narg);
lua_Integer luaL_optinteger(lua_State *L, int narg, lua_Integer def);

#define lua_isnumber(L,n)		(lua_type(L, (n)) == LUA_TNUMBER)

#define luaL_argcheck(L, cond,numarg,extramsg) ((void)((cond) || luaL_argerror(L, (numarg), (extramsg))))
#define luaL_checkstring(L,n)   (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)   (luaL_optlstring(L, (n), (d), NULL))
#define luaL_checkint(L,n)      ((int)luaL_checkinteger(L, (n)))
#define luaL_optint(L,n,d)      ((int)luaL_optinteger(L, (n), (d)))
#define luaL_checklong(L,n)     ((long)luaL_checkinteger(L, (n)))
#define luaL_optlong(L,n,d)     ((long)luaL_optinteger(L, (n), (d)))

#define luaL_opt(L,f,n,d)       (lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))
#define luaL_typename(L,i)      lua_typename(L, lua_type(L,(i)))

void luaL_checkstack(lua_State *L, int sz, const char *msg);
int lua_rawequal(lua_State *L, int idx1, int idx2);
