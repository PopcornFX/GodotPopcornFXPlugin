//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_scene.h"

#include "godot_cpp/classes/physics_direct_space_state2d.hpp"
#include "godot_cpp/classes/physics_direct_space_state3d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters2d.hpp"
#include "godot_cpp/classes/physics_ray_query_parameters3d.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/rendering_server.hpp"

#include "integration/pk_error_handling.h"
#include "integration/pk_plugin.h"
#include "integration/pk_plugin_types.h"
#include "pk_manager.h"
#include "pk_particles/include/ps_lod.h"

#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_storage.h>

PKScene::PKScene(PKRender::RenderMode p_render_mode) {
	render_mode = p_render_mode;
	particle_medium_collection = PK_NEW(CParticleMediumCollection(this));
	if (PKGD_VERIFY(particle_medium_collection != nullptr)) {
		particle_medium_collection->EnableBounds(true);
		particle_medium_collection->RegisterView();
		scene_views.PushBack();
	}
	if (p_render_mode == PKRender::RENDER_MODE_2D) {
		const TMemoryView<const LOD::SConfig::SRateLookup> rates;
		LOD::SConfig lod_config = LOD::SConfig(rates);
		lod_config.m_MinMinDist = 0;
		lod_config.m_MinDist = 0;
		lod_config.m_MaxDist = 0;
		particle_medium_collection->SetLODConfig(&lod_config);
	}
}

PKScene::~PKScene() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!PKGD_VERIFY_MSG(visual_instances.size() == 0, "PopcornFXScene being destroyed, but some visual instances remain!!")) {
		for (const auto &elem : visual_instances) {
			rs->free_rid(elem.value.first);
		}
		visual_instances.clear();
	}
	if (!PKGD_VERIFY_MSG(canvas_items.size() == 0, "PopcornFXScene being destroyed, but some canvas items remain!!")) {
		for (const auto &elem : canvas_items) {
			rs->free_rid(elem.value.first);
		}
		canvas_items.clear();
	}

	shader_materials.clear();
	sound_instances_3d.clear();
	sound_instances_2d.clear();
	PK_SAFE_DELETE(particle_medium_collection);
}

PKShaderMaterial &PKScene::get_or_create_shader_material(const CRendererDataBase *p_renderer) {
	if (const auto found = shader_materials.find(p_renderer); found != shader_materials.end()) {
		return found->value;
	}

	PKShaderMaterial new_mat;
	new_mat.init_from_renderer(*p_renderer, render_mode);
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

void PKScene::set_sound_instance_rendered_3d(PKAudioPlayerPool<AudioStreamPlayer3D> *p_audio_pool_ptr) {
	sound_instances_3d[p_audio_pool_ptr] = true;
}

void PKScene::set_sound_instance_rendered_2d(PKAudioPlayerPool<AudioStreamPlayer2D> *p_audio_pool_ptr) {
	sound_instances_2d[p_audio_pool_ptr] = true;
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

void PKScene::clear_sound_instance_3d(PKAudioPlayerPool<AudioStreamPlayer3D> *p_audio_pool_ptr) {
	if (sound_instances_3d.has(p_audio_pool_ptr)) {
		sound_instances_3d.erase(p_audio_pool_ptr);
	}
}

void PKScene::clear_sound_instance_2d(PKAudioPlayerPool<AudioStreamPlayer2D> *p_audio_pool_ptr) {
	if (sound_instances_2d.has(p_audio_pool_ptr)) {
		sound_instances_2d.erase(p_audio_pool_ptr);
	}
}

Pair<RID, bool> &PKScene::get_or_create_canvas_item(RID p_mesh_rid) {
	if (const auto found = canvas_items.find(p_mesh_rid); found != canvas_items.end()) {
		return found->value;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	RID new_instance = rs->canvas_item_create();

	return canvas_items.insert(p_mesh_rid, { new_instance, false })->value;
}

void PKScene::clear_canvas_item(RID p_mesh_rid) {
	const auto found = canvas_items.find(p_mesh_rid);
	if (found == canvas_items.end()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->canvas_item_clear(found->value.first);
	rs->free_rid(found->value.first);

	canvas_items.erase(p_mesh_rid);
}

void PKScene::update(float p_delta_time, CFloat4x4 &p_mat_wv, CFloat4x4 &p_mat_wvp) {
	scene_views[0].m_InvViewMatrix = p_mat_wv.Inverse();
	particle_medium_collection->UpdateView(0, p_mat_wv, p_mat_wvp);

	particle_medium_collection->Update(p_delta_time);
	particle_medium_collection->UpdateFence();

	_post_update_instances();
	_post_update_canvas_items();
}

void PKScene::RayTracePacket(const Colliders::STraceFilter &p_trace_filter, const Colliders::SRayPacket &p_packet, const Colliders::STracePacket &p_results) {
	if (state_3d != nullptr) {
		for (int i = 0; i < p_results.m_PacketSize; i++) {
			const Vector3 origin = to_gd(p_packet.m_RayOrigins_Aligned16[i].xyz());
			const Vector3 direction = to_gd(p_packet.m_RayDirectionsAndLengths_Aligned16[i].xyz());
			const float length = p_packet.m_RayDirectionsAndLengths_Aligned16[i].w();
			const Vector3 to = origin + direction * length;
			// Note: We're ignoring the radius parameter for now. If you want to implement swept-sphere casts, look into the ShapeCast3D node

			const Dictionary result = state_3d->intersect_ray(PhysicsRayQueryParameters3D::create(origin, to));
			if (result.is_empty()) {
				continue;
			}
			const Vector3 hit_position = Vector3(result["position"]);
			const Vector3 hit_normal = Vector3(result["normal"]);
			p_results.m_HitTimes_Aligned16[i] = Math::abs((origin - hit_position).length());
			if (p_results.m_ContactNormals_Aligned16 != nullptr) {
				p_results.m_ContactNormals_Aligned16[i].xyz() = to_pk(hit_normal);
			}
			if (p_results.m_ContactPoints_Aligned16 != nullptr) {
				p_results.m_ContactPoints_Aligned16[i].xyz() = to_pk(hit_position);
			}
			if (p_results.m_ContactObjects_Aligned16 != nullptr) {
				p_results.m_ContactObjects_Aligned16[i] = CollidableObject::DEFAULT;
			}
		}
	}
	if (state_2d != nullptr) {
		const float size = PKPlugin::get_singleton()->get_project_settings()->unit_pixel_size();
		for (int i = 0; i < p_results.m_PacketSize; i++) {
			Vector2 origin = to_gd(p_packet.m_RayOrigins_Aligned16[i].xy() * size);
			Vector2 direction = to_gd(p_packet.m_RayDirectionsAndLengths_Aligned16[i].xy() * size);
			direction.y *= -1;
			origin.y *= -1;
			const float length = p_packet.m_RayDirectionsAndLengths_Aligned16[i].w();
			Vector2 to = origin + direction * length;

			const Dictionary result = state_2d->intersect_ray(PhysicsRayQueryParameters2D::create(origin, to));
			if (result.is_empty()) {
				continue;
			}
			Vector2 hit_position = Vector2(result["position"]) / size; // No flipping y, Godot returns a "correct" y already.
			Vector2 hit_normal = Vector2(result["normal"]);
			hit_normal.y *= -1;
			p_results.m_HitTimes_Aligned16[i] = Math::abs((origin / size - hit_position).length());
			if (p_results.m_ContactNormals_Aligned16 != nullptr) {
				p_results.m_ContactNormals_Aligned16[i].xy() = to_pk(hit_normal);
				p_results.m_ContactNormals_Aligned16[i].z() = 0.f;
			}
			if (p_results.m_ContactPoints_Aligned16 != nullptr) {
				p_results.m_ContactPoints_Aligned16[i].xy() = to_pk(hit_position);
				p_results.m_ContactPoints_Aligned16[i].z() = 0.f;
			}
			if (p_results.m_ContactObjects_Aligned16 != nullptr) {
				p_results.m_ContactObjects_Aligned16[i] = CollidableObject::DEFAULT;
			}
		}
	}
}

void PKScene::_post_update_instances() {
	RenderingServer *rs = RenderingServer::get_singleton();

	// Visual instances
	for (auto it = visual_instances.begin(); it != visual_instances.end(); ++it) {
		if (it->value.second) {
			it->value.second = false;
		} else {
			// Batch drawer stopped updating: stop rendering it
			rs->instance_set_scenario(it->value.first, RID());
		}
	}

	// Sound pools
	for (auto it = sound_instances_3d.begin(); it != sound_instances_3d.end(); ++it) {
		if (it->value) {
			it->value = false;
		} else {
			// Batch drawer stopped updating: stop rendering it
			it->key->release_all();
		}
	}
}

void PKScene::_post_update_canvas_items() {
	RenderingServer *rs = RenderingServer::get_singleton();

	for (auto it = canvas_items.begin(); it != canvas_items.end(); ++it) {
		if (it->value.second) {
			it->value.second = false;
		} else {
			// Batch drawer stopped updating: stop rendering it
			rs->canvas_item_set_parent(it->value.first, RID());
		}
	}

	// Sound pools
	for (auto it = sound_instances_2d.begin(); it != sound_instances_2d.end(); ++it) {
		if (it->value) {
			it->value = false;
		} else {
			// Batch drawer stopped updating: stop rendering it
			it->key->release_all();
		}
	}
}

TMemoryView<const float *const> PKScene::GetAudioWaveform(CStringId p_channelGroup, u32 &r_base_count) const {
	return PKManager::get_singleton()->get_waveform(p_channelGroup, r_base_count);
}

TMemoryView<const float *const> PKScene::GetAudioSpectrum(CStringId p_channelGroup, u32 &r_base_count) const {
	return PKManager::get_singleton()->get_spectrum(p_channelGroup, r_base_count);
}
