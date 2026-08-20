//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_sdk.h"

#include "core/pk_buffer.h"
#include "core/pk_packed_byte_array.h"
#include "integration/engine/render/pk_render_type_policies.h"
#include "integration/engine/render/pk_render_utils.h"
#include "integration/engine/render/pk_renderer_cache.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_sound_std.h>

using namespace godot;

class PKBatchDrawerSoundCPU : public CRendererBatchJobs_Sound_Std {
	typedef CRendererBatchJobs_Sound_Std Super;

public:
	PKBatchDrawerSoundCPU();
	~PKBatchDrawerSoundCPU();

	virtual bool Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) override;

	virtual bool AreRenderersCompatible(const CRendererDataBase *p_renderer_a, const CRendererDataBase *p_renderer_b) const override;

	virtual bool CanRender(SRenderContext &p_ctx) const override;

	virtual void BeginFrame(SRenderContext &p_ctx) override;
	virtual bool EmitDrawCall(SRenderContext &p_ctx, const SDrawCallDesc &p_to_emit) override;

private:
	PKScene *owner_scene = nullptr;
	bool _render_audio(const Drawers::SSound_DrawRequest *p_sound_request, PKRendererCacheAudio *p_renderer_cache, const u32 p_particle_count);
};

//----------------------------------------------------------------------------
