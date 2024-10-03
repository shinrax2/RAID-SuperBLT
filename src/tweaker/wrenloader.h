#pragma once

#include <wren.hpp>

#include <mutex>

namespace raidhook::wren
{
	WrenVM* get_wren_vm();
	std::lock_guard<std::recursive_mutex> lock_wren_vm();
} // namespace raidhook::wren
