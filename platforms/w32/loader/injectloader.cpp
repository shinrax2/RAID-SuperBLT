// For the injectable BLT, we don't need to do anything except for loading the RAID hook
#ifdef INJECTABLE_BLT

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "InitState.h"

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		raidhook::InitiateStates();
	}

	return 1;
}

#endif
