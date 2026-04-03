//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/variant/rid.hpp"

#include "integration/pk_sdk.h"

#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

using namespace godot;

struct PKRenderContext : SRenderContext {
	RID scenario_id;
};
