//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_sdk.h"

#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>

class PKRenderDataFactory {
public:
	PKRenderDataFactory() = default;
	~PKRenderDataFactory() = default;

	CRendererBatchDrawer *create_batch_drawer(ERendererClass p_renderer_type, const PRendererCacheBase &p_renderer_cache, bool p_gpu_storage);
	PRendererCacheBase create_renderer_cache(const PRendererDataBase &p_renderer, const CParticleDescriptor *p_particle_desc);
};
