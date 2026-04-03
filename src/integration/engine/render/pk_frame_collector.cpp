//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_frame_collector.h"

bool PKFrameCollector::EarlyCull(const CAABB &p_bbox) const {
	// Can happen if bounds are not active
	if (!p_bbox.IsFinite() ||
			!p_bbox.Valid()) {
		return false;
	}
	return false;
}
