//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#undef PV_MODULE_INIT_NAME
#undef PV_MODULE_NAME
#undef PV_MODULE_SYM
#define PV_MODULE_INIT_NAME "Godot PopcornFX Plugin"
#define PV_MODULE_NAME "Godot"
#define PV_MODULE_SYM Godot

#include <pkapi/include/pk_precompiled_default.h>

// This header leaks the Windows headers
#include <pk_kernel/include/kr_threads_basics.h>

// This header undefs some Windows defines used by Godot
#include "godot_cpp/core/defs.hpp"

PK_LOG_MODULE_DEFINE();

using namespace PopcornFX;
