//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/variant/rid.hpp"

#include "integration/pk_sdk.h"

#include "core/pk_shader_material.h"

#include <pk_particles/include/ps_scene.h>
#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

namespace PopcornFX {
PK_FORWARD_DECLARE(ParticleMediumCollection);
}
using namespace godot;

class PKScene : public IParticleScene {
public:
	PKScene();
	~PKScene();

	CParticleMediumCollection *get_particle_medium_collection() { return particle_medium_collection; }
	TArray<SSceneView> &get_views() { return scene_views; }

	const PKShaderMaterial &get_or_create_shader_material(const CRendererDataBase *p_renderer);
	void clear_shader_material(const CRendererDataBase *p_renderer);

	Pair<RID, bool> &get_or_create_visual_instance(RID p_mesh_rid);
	void clear_visual_instance(RID p_mesh_rid);

	void update(float p_delta_time, CFloat4x4 &p_mat_wv, CFloat4x4 &p_mat_wvp);

	PhysicsDirectSpaceState3D *state;
	virtual void RayTracePacket(const Colliders::STraceFilter &p_trace_filter, const Colliders::SRayPacket &p_packet, const Colliders::STracePacket &p_results) override;
	virtual TMemoryView<const float *const> GetAudioWaveform(CStringId p_channel_group, PopcornFX::u32 &r_base_count) const override;
	virtual TMemoryView<const float *const> GetAudioSpectrum(CStringId p_channel_group, PopcornFX::u32 &r_base_count) const override;

private:
	void _post_update_instances();

	CParticleMediumCollection *particle_medium_collection = nullptr;
	TArray<SSceneView> scene_views;
	HashMap<const CRendererDataBase *, PKShaderMaterial> shader_materials;
	HashMap<RID, Pair<RID, bool>> visual_instances; // Key: mesh RID, Value: visual_instance RID & rendered_this_frame bool
};
