//
// Created by znix on 08/05/2021.
//

#pragma once

#include <wren.hpp>

namespace raidhook::tweaker::wren_utils
{
	WrenForeignMethodFn bind_wren_utils_method(WrenVM* vm, const char* module, const char* className, bool isStatic,
	                                           const char* signature);
} // namespace raidhook::tweaker::wren_utils
