//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "core/pk_buffer.h"

#include "core/pk_audio_player_pool.h"
#include "integration/pk_sdk.h"

#include <pk_render_helpers/include/frame_collector/rh_particle_render_data_factory.h>

struct PKParticleMaterialDescSound {
	StringName sound_path;

	PKAudioPlayerPool<AudioStreamPlayer3D> pool; // TODO modify when 2D

	bool init_from_renderer(const CRendererDataSound &p_renderer);

	bool operator==(const PKParticleMaterialDescSound &p_other) const;
};

class PKRendererCacheAudio : public CRendererCacheBase {
public:
	PKParticleMaterialDescSound material_desc_sound;

	PKRendererCacheAudio() = default;
	~PKRendererCacheAudio();

	virtual void UpdateThread_BuildBillboardingFlags(const PRendererDataBase &p_renderer) override;

	bool setup_renderer_sound(const CRendererDataSound *p_renderer);

	bool operator==(const PKRendererCacheAudio &p_other) const;
};

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
