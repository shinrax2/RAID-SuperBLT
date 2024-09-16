#pragma once

#ifdef SIG_INCLUDE_MAIN
#include "signatures.h"

#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset);

#define CREATE_CALLABLE_CLASS_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
	name ## ptr name = NULL; \
	SignatureSearch name ## search(#name, &name, signature, mask, offset);

#else

// If we're not being included directly from InitiateState.cpp, only declare, not define, variables
#define CREATE_NORMAL_CALLABLE_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(*name ## ptr)(__VA_ARGS__); \
	extern name ## ptr name;

#define CREATE_CALLABLE_CLASS_SIGNATURE(name, retn, signature, mask, offset, ...) \
	typedef retn(__thiscall *name ## ptr)(void*, __VA_ARGS__); \
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



#include "signatures/sigdef_game.h"


// The four different try_open functions, each one with a different resolver to filter files
// Unfortunately, the language, english, and funcptr resolvers are identical bar for a different function call
// address, so we can't hook them normally. They have special handling by means of the FindAssetLoadSignatures
// function. Their addresses then get filled out here:
// (note: ifdef'd off so we don't have to include vector everywhere)
#ifdef INCLUDE_TRY_OPEN_FUNCTIONS
extern std::vector<void*> try_open_functions;
#endif

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
