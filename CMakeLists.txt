cmake_minimum_required(VERSION 3.18)

project(SuperBLT)

###############################################################################
## misc. definitions ##########################################################
###############################################################################

enable_language(ASM_MASM)

# Use CXX 20
set(CMAKE_CXX_STANDARD 20)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE	sources		src/*.cpp src/*.c src/*.h)

set(				win32_all_game_path			platforms/w32/game)
set(				win32_game_path				${win32_all_game_path}/raid)
file(GLOB_RECURSE	all_game_win32_sources	${win32_all_game_path}/*.cpp ${win32_all_game_path}/*.h)
file(GLOB_RECURSE	game_win32_sources		${win32_game_path}/*.cpp ${win32_game_path}/*.h)
# Only Payday 2 has a linux build at the moment so we don't really need game specific stuff for it. 

file(GLOB_RECURSE	win32_sources		platforms/w32/*.cpp platforms/w32/*.h)
set(				win32_loader_path	platforms/w32/loader)

file(GLOB_RECURSE	win32_loaders		${win32_loader_path}/*.cpp)
list(REMOVE_ITEM	win32_sources		${win32_loaders})
list(REMOVE_ITEM	win32_sources		${all_game_win32_sources})
list(APPEND			win32_sources		${game_win32_sources})

# you can use set(sources src/main.cpp) etc if you don't want to
# use globing to find files automatically

###############################################################################
## target definitions #########################################################
###############################################################################

set(mxml_sources mxml-attr.c mxml-entity.c mxml-file.c mxml-get.c mxml-index.c
	mxml-node.c mxml-search.c mxml-set.c mxml-private.c mxml-string.c)
list(TRANSFORM mxml_sources PREPEND lib/mxml/)
add_library(mxml STATIC ${mxml_sources})
target_include_directories(mxml PRIVATE lib/configs/mxml)
target_compile_options(mxml PRIVATE -D_CRT_SECURE_NO_WARNINGS)
target_include_directories(mxml PUBLIC lib/mxml)

file(GLOB_RECURSE wren_sources
	# Public source file
	lib/wren/src/include/wren.h 
	# Core VM files
	lib/wren/src/vm/*.c
	lib/wren/src/vm/*.h
	# Optional components (Meta and Random)
	lib/wren/src/optional/*.c
	lib/wren/src/optional/*.h
)
add_library(wren STATIC ${wren_sources})
target_include_directories(wren PRIVATE lib/wren/src/optional lib/wren/src/vm)
target_compile_options(wren PRIVATE -D_CRT_SECURE_NO_WARNINGS)
target_include_directories(wren PUBLIC lib/wren/src/include)

add_library(subhook STATIC lib/subhook/subhook.c)
target_compile_options(subhook PRIVATE -DSUBHOOK_EXPORT=;)
target_include_directories(subhook PUBLIC lib/subhook)

###############################################################################
## Wren source generation #####################################################
###############################################################################

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(wren_src_files
	wren/native.wren
	wren/DB_001.wren
	wren/internal_001.wren
	wren/LuaInterface_001.wren
	wren/Environment_001.wren
	wren/Utils_001.wren
)
add_custom_command(
	OUTPUT wren_generated_src.c gen/wren_generated_src.h
	COMMAND ${Python3_EXECUTABLE} wren/generate.py ${CMAKE_CURRENT_BINARY_DIR} ${wren_src_files}
	MAIN_DEPENDENCY wren/generate.py
	DEPENDS ${wren_src_files}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	VERBATIM
)
list(APPEND sources ${CMAKE_CURRENT_BINARY_DIR}/wren_generated_src.c)

###############################################################################
## Base (non-platform-specific) part of SuperBLT ##############################
###############################################################################

add_library(SuperBLT STATIC ${sources} ${win32_sources} platforms/w32/platform_xml.asm)
set_property(SOURCE platforms/w32/platform_xml.asm PROPERTY LANGUAGE ASM_MASM)

target_compile_options(SuperBLT PRIVATE
	-DAL_LIBTYPE_STATIC
	-DCURL_STATICLIB
	-DSUBHOOK_STATIC
)

# General optimisation breaks calls to certain lua functions, so replace it.
# We statically link to reduce dependencies
foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	if(${flag_var} MATCHES "/O2")
		string(REGEX REPLACE "/O2" "/Od /Oi /Ot /Oy /Ob2 /GF /Gy" ${flag_var} "${${flag_var}}")
	endif(${flag_var} MATCHES "/O2")
endforeach(flag_var)

# Allow both SuperBLT and the loaders access to the headers
target_include_directories(SuperBLT PUBLIC src)

# Make the generated wren header available
target_include_directories(SuperBLT PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)

# Give access to game specific platforms
target_include_directories(SuperBLT PUBLIC ${win32_game_path})

###############################################################################
## dependencies ###############################################################
###############################################################################

target_link_libraries(SuperBLT mxml)
target_link_libraries(SuperBLT wren)
target_link_libraries(SuperBLT subhook)

#### zlib ####

# # zlib requires an old version of cmake before target_include_directories was added
add_subdirectory(lib/sys/zlib EXCLUDE_FROM_ALL)
target_include_directories(zlibstatic PUBLIC ${zlib_BINARY_DIR} ${zlib_SOURCE_DIR})
target_link_libraries(SuperBLT zlibstatic)

#### CURL ####

# Required to build curl as static
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build as static, set by SBLT's buildscript")

# Disable all the install targets, otherwise it causes errors when we add dependencies to curl
function(install)
endfunction()

# Use Windows SSPI for HTTPS
set(CURL_USE_SCHANNEL ON)

# Turn off all the crap we don't want
set(CURL_DISABLE_INSTALL ON)
set(HTTP_ONLY ON) # disable all protocols, but http(s) and ws(s)
set(ENABLE_UNIX_SOCKETS OFF) # disable unit-socket support [ http(s):// ]
set(CURL_DISABLE_NEGOTIATE_AUTH ON) # included in SSPI
set(CURL_DISABLE_KERBEROS_AUTH ON) # included in SSPI
set(CURL_DISABLE_NTLM ON) # included in SSPI
set(CURL_DISABLE_WEBSOCKETS ON) # disable websocket support [ ws(s):// ]

# TODO enable zlib
set(CURL_USE_LIBPSL OFF) # TODO: enable this too

add_subdirectory(lib/sys/curl EXCLUDE_FROM_ALL)
target_link_libraries(SuperBLT libcurl_static)

#### LuaJIT ####

add_subdirectory(src/luajit EXCLUDE_FROM_ALL)
target_link_libraries(SuperBLT lua51)

###############################################################################
## loader binary (this generates a seperate target for both Windows loaders) ##
###############################################################################

# Define the macro
macro(Add_Loader library_name)
	add_library(${library_name} SHARED ${ARGN})
	target_link_libraries(${library_name} LINK_PRIVATE SuperBLT)
endmacro()

Add_Loader("IPHLPAPI" ${win32_loader_path}/loader.asm ${win32_loader_path}/iphp.cpp ${win32_loader_path}/iphp.def)
Add_Loader("WSOCK32" ${win32_loader_path}/loader.asm ${win32_loader_path}/wsock.cpp ${win32_loader_path}/wsock.def)

set_property(SOURCE ${win32_loader_path}/loader.asm PROPERTY LANGUAGE ASM_MASM)
set_property(SOURCE ${win32_loader_path}/loader.asm PROPERTY LANGUAGE ASM_MASM)

target_compile_options(IPHLPAPI PRIVATE -DBLT_USE_IPHLPAPI)
target_compile_options(WSOCK32 PRIVATE -DBLT_USE_WSOCK)

