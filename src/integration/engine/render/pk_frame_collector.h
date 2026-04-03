//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_sdk.h"

#include <pk_render_helpers/include/frame_collector/rh_frame_collector.h>

// Frame collector is templated with user data:
class PKFrameCollector : public CFrameCollector {
public:
	PKFrameCollector() = default;
	~PKFrameCollector() = default;

private:
	// Early Cull: Culls an entire medium on the update thread (when collecting the frame)
	virtual bool EarlyCull(const CAABB &p_bbox) const override;

	// Late Cull: Cull draw requests or individual pages on the render thread (when collecting draw calls)
	// You can use this method if you don't have render thread views available from the update thread
	// Ideally, cull in EarlyCull, but you should implement both methods
	// Late cull also allows for finer culling (per draw request / per page)
	virtual bool LateCull(const CAABB &p_bbox) const override { return EarlyCull(p_bbox); }
};
