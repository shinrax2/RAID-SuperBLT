#pragma once

#define portable_strncpy(dest, source, length) strncpy_s(dest, length, source, length)
#define strdup _strdup
#define localtime_r(time, out) localtime_s(out, time)
