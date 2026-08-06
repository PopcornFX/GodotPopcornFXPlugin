//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_render_data_factory.h"

#include "integration/pk_error_handling.h"
#include "pk_batch_drawer_billboard_cpu.h"
#include "pk_batch_drawer_mesh_cpu.h"
#include "pk_batch_drawer_ribbon_cpu.h"
#include "pk_batch_drawer_sound_cpu.h"
#include "pk_renderer_cache.h"

CRendererBatchDrawer *PKRenderDataFactory::create_batch_drawer(ERendererClass p_renderer_type, const PRendererCacheBase &p_renderer_cache, bool p_gpu_storage) {
	(void)p_renderer_cache;
	if (!p_gpu_storage) {
		switch (p_renderer_type) {
			case Renderer_Billboard:
				return PK_NEW(PKBatchDrawerBillboardCPU);
			case Renderer_Ribbon:
				return PK_NEW(PKBatchDrawerRibbonCPU);
			case Renderer_Mesh:
				return PK_NEW(PKBatchDrawerMeshCPU);
			case Renderer_Sound:
				return PK_NEW(PKBatchDrawerSoundCPU);
			//case	Renderer_Light:
			//{
			//	GodotLightBatchDrawer *batch = PK_NEW(GodotLightBatchDrawer);
			//	return batch;
			//}
			default:
				return nullptr;
		}
	} else {
		PK_ASSERT_NOT_IMPLEMENTED_MESSAGE("PopcornFX GPU simulation is not implemented in Godot yet...");
	}
	return nullptr;
}

PRendererCacheBase PKRenderDataFactory::create_renderer_cache(const PRendererDataBase &p_renderer, const CParticleDescriptor *p_particle_desc) {
	// This is called when the effect is preloaded, or when first instantiated
	// It is required that a renderer cache is created per renderer.

	// Returned value is PRendererCacheBase (strong ref), you can leave PopcornFX handle the cache's lifetime (no need to free it)
	// It will be destroyed when effect is unloaded.

	// We create the renderer cache here:
	CRendererCacheBase *renderer_cache;

	switch (p_renderer->m_RendererType) {
		case Renderer_Sound: {
			renderer_cache = PK_NEW(PKRendererCacheAudio);
			break;
		}
		default: {
			renderer_cache = PK_NEW(PKRendererCache);
			break;
		}
	}

	if (!PKGD_VERIFY(renderer_cache != nullptr)) {
		//Error("PopcornFX", false, "Could not allocate the renderer cache (effect: '%s')", particleDesc->ParentEffect()->FilePath().Data());
		return nullptr;
	}

	if (p_renderer->m_RendererType == Renderer_Sound) {
		(static_cast<PKRendererCacheAudio *>(renderer_cache))->setup_renderer_sound(static_cast<const CRendererDataSound *>(p_renderer.Get()));
	}

	return renderer_cache;
}
