//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "core/pk_buffer.h"

#include "integration/pk_sdk.h"

#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>

class PKRendererCache : public CRendererCacheBase {
public:
	PKRendererCache() = default;
	~PKRendererCache();

	virtual void UpdateThread_BuildBillboardingFlags(const PRendererDataBase &p_renderer) override;
	virtual bool UpdateThread_LoadRendererAtlas(const PRendererDataBase &p_renderer, CResourceManager *p_resource_manager) override;

	Ref<Texture2DRD> get_atlas_rects_srv() const;

	bool operator==(const PKRendererCache &p_other) const;

private:
	PKBuffer atlas_rects;
};
