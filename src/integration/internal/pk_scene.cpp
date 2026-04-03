//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_scene.h"

#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/rendering_server.hpp"

#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"
#include "pk_manager.h"

#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_storage.h>

PKScene::PKScene() {
	particle_medium_collection = PK_NEW(CParticleMediumCollection(this));
	if (PKGD_VERIFY(particle_medium_collection != nullptr)) {
		particle_medium_collection->EnableBounds(true);
		particle_medium_collection->RegisterView();
		scene_views.PushBack();
	}
}

PKScene::~PKScene() {
	if (!PKGD_VERIFY_MSG(visual_instances.size() == 0, "PopcornFXScene being destroyed, but some visual instances remain!!")) {
		RenderingServer *rs = RenderingServer::get_singleton();
		for (const auto &elem : visual_instances) {
			rs->free_rid(elem.value.first);
		}
		visual_instances.clear();
	}

	PK_SAFE_DELETE(particle_medium_collection);
}

const PKShaderMaterial &PKScene::get_or_create_shader_material(const CRendererDataBase *p_renderer) {
	if (const auto found = shader_materials.find(p_renderer); found != shader_materials.end()) {
		return found->value;
	}

	PKShaderMaterial new_mat;
	new_mat.init_from_renderer(*p_renderer);
	new_mat.create_material_instance();
	return shader_materials.insert(p_renderer, new_mat)->value;
}

void PKScene::clear_shader_material(const CRendererDataBase *p_renderer) {
	shader_materials.erase(p_renderer);
}

Pair<RID, bool> &PKScene::get_or_create_visual_instance(RID p_mesh_rid) {
	if (const auto found = visual_instances.find(p_mesh_rid); found != visual_instances.end()) {
		return found->value;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	RID new_instance = rs->instance_create();

	return visual_instances.insert(p_mesh_rid, { new_instance, false })->value;
}

void PKScene::clear_visual_instance(RID p_mesh_rid) {
	const auto found = visual_instances.find(p_mesh_rid);
	if (found == visual_instances.end()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->free_rid(found->value.first);

	visual_instances.erase(p_mesh_rid);
}

void PKScene::update(float p_delta_time, CFloat4x4 &p_mat_wv, CFloat4x4 &p_mat_wvp) {
	scene_views[0].m_InvViewMatrix = p_mat_wv.Inverse();
	particle_medium_collection->UpdateView(0, p_mat_wv, p_mat_wvp);

	particle_medium_collection->Update(p_delta_time);
	particle_medium_collection->UpdateFence();

	_post_update_instances();
}

void PKScene::RayTracePacket(const Colliders::STraceFilter &p_trace_filter, const Colliders::SRayPacket &p_packet, const Colliders::STracePacket &p_results) {
	ERR_FAIL_NULL(state);

	for (int i = 0; i < p_results.m_PacketSize; i++) {
		const Vector3 origin = to_gd(p_packet.m_RayOrigins_Aligned16[i].xyz());
		const Vector3 direction = to_gd(p_packet.m_RayDirectionsAndLengths_Aligned16[i].xyz());
		const float length = p_packet.m_RayDirectionsAndLengths_Aligned16[i].w();
		const Vector3 to = origin + direction * length;
		// Note: We're ignoring the radius parameter for now. If you want to implement swept-sphere casts, look into the ShapeCast3D node

		const Dictionary result = state->intersect_ray(PhysicsRayQueryParameters3D::create(origin, to));
		if (result.is_empty()) {
			continue;
		}
		const Vector3 hit_position = Vector3(result["position"]);
		p_results.m_HitTimes_Aligned16[i] = Math::abs((origin - hit_position).length());
		if (p_results.m_ContactNormals_Aligned16 != nullptr) {
			p_results.m_ContactNormals_Aligned16[i].xyz() = to_pk(Vector3(result["normal"]));
		}
		if (p_results.m_ContactPoints_Aligned16 != nullptr) {
			p_results.m_ContactPoints_Aligned16[i].xyz() = to_pk(hit_position);
		}
		if (p_results.m_ContactObjects_Aligned16 != nullptr) {
			p_results.m_ContactObjects_Aligned16[i] = CollidableObject::DEFAULT;
		}
	}
}

void PKScene::_post_update_instances() {
	RenderingServer *rs = RenderingServer::get_singleton();

	for (auto it = visual_instances.begin(); it != visual_instances.end(); ++it) {
		if (it->value.second) {
			it->value.second = false;
			continue;
		}

		// Batch drawer stopped updating: stop rendering it
		rs->instance_set_scenario(it->value.first, RID());
	}
}

TMemoryView<const float *const> PKScene::GetAudioWaveform(CStringId p_channelGroup, u32 &r_base_count) const {
	return PKManager::get_singleton()->get_waveform(p_channelGroup, r_base_count);
}

TMemoryView<const float *const> PKScene::GetAudioSpectrum(CStringId p_channelGroup, u32 &r_base_count) const {
	return PKManager::get_singleton()->get_spectrum(p_channelGroup, r_base_count);
}
