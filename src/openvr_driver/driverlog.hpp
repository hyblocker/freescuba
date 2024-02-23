//============ Copyright (c) Valve Corporation, All rights reserved. ============
#pragma once

#include <string>
#include <openvr_driver.h>

extern void DriverLog(const char* pchFormat, ...);

extern void DebugDriverLog(const char* pchFormat, ...);

#define LOG(fmt, ...) DriverLog(fmt, __VA_ARGS__)
#define TRACE(fmt, ...) DebugDriverLog(fmt, __VA_ARGS__)