//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_batch_drawer_mesh_cpu.h"

#include "godot_cpp/classes/box_shape3d.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/hash_set.hpp"

#include "integration/engine/render/pk_frame_collector_types.h"
#include "integration/engine/render/pk_renderer_cache.h"
#include "integration/internal/pk_project_settings.h"
#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"
#include "pk_manager.h"

#include <pk_maths/include/pk_maths_simd_matrix.h>
#include <pk_maths/include/pk_maths_simd_vector.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

using RS = RenderingServer;

// TODO: Reevaluate implementing 2D mesh renderers.
// Godot currently has poor support for 3D meshes in 2D scenes. Such meshes cannot be rotated on the 3D axes, instead acting as a sprite with z rotation and skew.
// Additionally, the lack of depth buffer creates issues with rendering textures on these meshes, requiring triangle sorting/OIT to render properly.
// These two big issues accumulate into making 3D mesh support quite basic in 2D.
// If we one day want to implement better support, the easiest path may be to add support in Godot for multimeshes with 3D transforms to be rendered in the canvas renderer.
// Though I (Arthur) haven't done any exploration on that, so it may be more annoying that I've guessed.

bool PKBatchDrawerMeshCPU::Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) {
	if (!Super::Setup(p_renderer, p_owner, p_fc, p_storage_class)) {
		return false;
	}

	if (render_mode == PKRender::RENDER_MODE_3D) {
		owner_scene = PKPlugin::get_singleton()->get_scene_3d();
	} else {
		owner_scene = PKPlugin::get_singleton()->get_scene_2d();
	}

	if (!PKGD_VERIFY(owner_scene != nullptr)) {
		return false;
	}

	RS *rs = RS::get_singleton();

	// Create material
	shader_mat = &owner_scene->get_or_create_shader_material(p_renderer);
	if (shader_mat->mesh == nullptr) {
		return false;
	}

	// Init mesh, then create and link with visual instance or canvas item
	multimesh = rs->multimesh_create();
	rs->multimesh_set_mesh(multimesh, shader_mat->mesh->get_rid());

	if (render_mode == PKRender::RENDER_MODE_3D) {
		const RID instance = owner_scene->get_or_create_visual_instance(multimesh).first;
		rs->instance_set_base(instance, multimesh);
		shader_mat->apply_to_visual_instance(instance);
	}

	if (render_mode == PKRender::RENDER_MODE_2D) {
		const RID canvas_item = owner_scene->get_or_create_canvas_item(multimesh).first;
		rs->canvas_item_add_multimesh(canvas_item, multimesh);
		rs->canvas_item_set_material(canvas_item, shader_mat->material->get_rid());
	}

#ifdef PK_DEBUG_AABB
	aabb_debug.init(owner_scene);
#endif

	sim_data.init();
	return true;
}

void PKBatchDrawerMeshCPU::Destroy() {
	if (owner_scene != nullptr) {
		if (render_mode == PKRender::RENDER_MODE_3D) {
			owner_scene->clear_visual_instance(multimesh);
		} else {
			owner_scene->clear_canvas_item(multimesh);
		}
#ifdef PK_DEBUG_AABB
		aabb_debug.destroy(owner_scene);
#endif
	}

	RS::get_singleton()->free_rid(multimesh);
	sim_data.destroy();
}

bool PKBatchDrawerMeshCPU::AreRenderersCompatible(const CRendererDataBase *p_renderer_a, const CRendererDataBase *p_renderer_b) const {
	if (!Super::AreRenderersCompatible(p_renderer_a, p_renderer_b)) {
		return false;
	}

	if (render_mode == PKRender::RENDER_MODE_2D) {
		const PKRendererCache *renderer_cache_a = static_cast<PKRendererCache *>(p_renderer_a->m_RendererCache.Get());
		const int z_index_a = PKManager::get_singleton()->get_effect_z_index(renderer_cache_a->get_effect_path());
		const PKRendererCache *renderer_cache_b = static_cast<PKRendererCache *>(p_renderer_b->m_RendererCache.Get());
		const int z_index_b = PKManager::get_singleton()->get_effect_z_index(renderer_cache_b->get_effect_path());
		if (z_index_a != z_index_b) {
			return false;
		}
	}

	const PKShaderMaterial &shader_a = owner_scene->get_or_create_shader_material(p_renderer_a);
	const PKShaderMaterial &shader_b = owner_scene->get_or_create_shader_material(p_renderer_b);
	return shader_a == shader_b;
}

bool PKBatchDrawerMeshCPU::AllocBuffers(SRenderContext &p_ctx) {
	if (!PKGD_VERIFY(m_DrawPass != nullptr)) {
		return false;
	}

	PKGD_ASSERT(m_DrawPass->m_TotalParticleCount == m_BB_Mesh.TotalParticleCount());
	particle_count = m_DrawPass->m_TotalParticleCount;

	instanced_matrices.Resize(particle_count);

	int transform_width = 12;
	RS::MultimeshTransformFormat transform_format = RS::MULTIMESH_TRANSFORM_3D;
	if (render_mode == PKRender::RENDER_MODE_2D) {
		transform_width = 8;
		transform_format = RS::MULTIMESH_TRANSFORM_2D;
	}
	instanced_transforms.resize(particle_count * sizeof(float) * transform_width);

	RS::get_singleton()->multimesh_allocate_data(multimesh, particle_count, transform_format);

	// Additional inputs
	if (m_DrawPass->m_IsNewFrame) {
		const uint32_t add_input_count = m_DrawPass->m_ToGenerate.m_AdditionalGeneratedInputs.Count();

		_clear_stream_offsets();
		additional_inputs.Clear();
		if (!PKGD_VERIFY(additional_inputs.Reserve(add_input_count))) { // Max possible additional input count
			return false;
		}

		uint32_t sim_data_size_bytes = 0;
		for (uint32_t i_input = 0; i_input < add_input_count; ++i_input) {
			const SRendererFeatureFieldDefinition &additional_input = m_DrawPass->m_ToGenerate.m_AdditionalGeneratedInputs[i_input];

			const uint32_t type_size = CBaseTypeTraits::Traits(additional_input.m_Type).Size;
			AdditionalStreamOffsets stream_offset_type = AdditionalStreamOffsets::STREAM_OFFSET_MAX;

			if (!_is_additional_input_supported(additional_input.m_Name, additional_input.m_Type, stream_offset_type)) {
				continue; // Unsupported shader input, discard
			}
			PKGD_ASSERT(stream_offset_type < AdditionalStreamOffsets::STREAM_OFFSET_MAX);

			if (!PKGD_VERIFY(additional_inputs.PushBack().Valid())) {
				return false;
			}
			PKAdditionalInput &new_add_input = additional_inputs.Last();
			new_add_input.additional_input_index = i_input;
			new_add_input.buffer_offset = sim_data_size_bytes;
			new_add_input.byte_size = type_size;
			additional_stream_offsets[stream_offset_type] = sim_data_size_bytes;

			sim_data_size_bytes += Mem::Align(type_size * particle_count, 8);
		}

		if (sim_data_size_bytes == 0) {
			sim_data_size_bytes = sizeof(float);
		}
		sim_data.resize(sim_data_size_bytes);
	}
	return true;
}

bool PKBatchDrawerMeshCPU::MapBuffers(SRenderContext &p_ctx) {
	(void)p_ctx;
	PKGD_ASSERT(m_DrawPass != nullptr);

	// Matrix billboarding
	m_BBJobs_Mesh.m_Exec_Matrices.m_Matrices = instanced_matrices.ViewForWriting();

	// Additional inputs
	if (!m_DrawPass->m_ToGenerate.m_AdditionalGeneratedInputs.Empty()) {
		const uint32_t add_input_count = additional_inputs.Count();

		uint8_t *sim_data_ptr = sim_data.map();

		if (sim_data_ptr == nullptr) {
			return false;
		}

		if (!PKGD_VERIFY(additional_inputs_mapped.Resize(add_input_count))) {
			return false;
		}
		for (uint32_t i_input = 0; i_input < add_input_count; ++i_input) {
			PKAdditionalInput &input = additional_inputs[i_input];
			Drawers::SCopyFieldDesc &desc = additional_inputs_mapped[i_input];

			desc.m_Storage.m_Count = particle_count;
			desc.m_Storage.m_RawDataPtr = Mem::AdvanceRawPointer(sim_data_ptr, input.buffer_offset);
			desc.m_Storage.m_Stride = input.byte_size;
			desc.m_AdditionalInputIndex = input.additional_input_index;
		}

		m_BBJobs_Mesh.m_Exec_CopyField.m_FieldsToCopy = additional_inputs_mapped.View();
	}
	return true;
}

bool PKBatchDrawerMeshCPU::UnmapBuffers(SRenderContext &p_ctx) {
	float *mapped_transforms = reinterpret_cast<float *>(instanced_transforms.ptrw());
	const CFloat4x4 *matrices_ptr = instanced_matrices.RawDataPointer();
	if (render_mode == PKRender::RENDER_MODE_3D) {
		for (uint32_t i = 0; i < particle_count; ++i) {
			float *ptr = mapped_transforms + i * 12;
			SIMD::Float4 row0, row1, row2, row3;
			SIMD::LoadAligned16_Transpose4x4(row0, row1, row2, row3, &matrices_ptr[i]);
			row0.StoreUnaligned(ptr, 0x00);
			row1.StoreUnaligned(ptr, 0x10);
			row2.StoreUnaligned(ptr, 0x20);
		}
	} else {
		// Rescale and pack transform matrices from popcorn format to Godot format
		// {x0x1x2x3, y0y1y2y3, z0z1z2z3, w0w1w2w3} -> {x0y0z0w0x1y1z1w1}
		const float unit_pixel_size = PKPlugin::get_singleton()->get_project_settings()->unit_pixel_size();
		constexpr uint32_t single_neg_mask = uint32_t(1) << 31;
		const SIMD::Float4 neg_mask_0 = SIMD::Float4::FromConstInt<single_neg_mask, 0, 0, 0>();
		const SIMD::Float4 neg_mask_1 = SIMD::Float4::FromConstInt<0, single_neg_mask, 0, single_neg_mask>();
		for (uint32_t i = 0; i < particle_count; ++i) {
			SIMD::Float4 rx = SIMD::Float4::LoadAligned16(&matrices_ptr[i], 0x00);
			SIMD::Float4 ry = SIMD::Float4::LoadAligned16(&matrices_ptr[i], 0x10);
			SIMD::Float4 rz = SIMD::Float4::LoadAligned16(&matrices_ptr[i], 0x20);
			SIMD::Float4 rw = SIMD::Float4::LoadAligned16(&matrices_ptr[i], 0x30);
			SIMD::Float4 x0y0x1y1 = SIMD::InterleaveLo(rx, ry);
			SIMD::Float4 z0w0z1w1 = SIMD::InterleaveLo(rz, rw);

			SIMD::Float4 x0y0z0w0 = SIMD::MoveLoHi(x0y0x1y1, z0w0z1w1);
			SIMD::Float4 x1y1z1w1 = SIMD::MoveHiLo(z0w0z1w1, x0y0x1y1);

			x0y0z0w0 *= unit_pixel_size;
			x1y1z1w1 *= unit_pixel_size;
			x0y0z0w0 ^= neg_mask_0;
			x1y1z1w1 ^= neg_mask_1;

			float *ptr = mapped_transforms + i * 8;
			x0y0z0w0.StoreUnaligned(ptr);
			x1y1z1w1.StoreUnaligned(ptr, 0x10);
		}
	}

	sim_data.unmap();
	return true;
}

bool PKBatchDrawerMeshCPU::EmitDrawCall(SRenderContext &p_ctx, const SDrawCallDesc &p_to_emit) {
	if (owner_scene == nullptr) {
		return false;
	}

	const PKRenderContext &render_context = static_cast<PKRenderContext &>(p_ctx);
	const PKRendererCache *renderer_cache = static_cast<PKRendererCache *>(p_to_emit.m_RendererCaches.First().Get());
	if (!PKGD_VERIFY(renderer_cache != nullptr)) {
		return true;
	}

	RS *rs = RS::get_singleton();

	// Set the bounding box of the batch drawer for culling
	AABB mesh_bbox = to_gd(p_to_emit.m_BBox);
	if (render_mode == PKRender::RENDER_MODE_2D) {
		// Resize mesh bbox & convert position
		const float unit_pixel_size = PKPlugin::get_singleton()->get_project_settings()->unit_pixel_size();
		mesh_bbox = to_gd(p_to_emit.m_BBox * unit_pixel_size);
		mesh_bbox.position.y = -mesh_bbox.position.y - mesh_bbox.size.y;

		// Apply bbox as a custom rect directly
		const RID canvas_item = owner_scene->get_or_create_canvas_item(multimesh).first;
		rs->canvas_item_set_custom_rect(canvas_item, true, Rect2(Vector2(mesh_bbox.position.x, mesh_bbox.position.y), Vector2(mesh_bbox.size.x, mesh_bbox.size.y)));

		// Compute z-index
		int z_index = PKManager::get_singleton()->get_effect_z_index(renderer_cache->get_effect_path());
		z_index += shader_mat->transparent_sort_override;
		rs->canvas_item_set_z_index(canvas_item, z_index);
	}

	rs->multimesh_set_custom_aabb(multimesh, mesh_bbox);

	// Directly update the multimesh's GPU instance transforms buffer with our data
	const RID instances_buffer = rs->multimesh_get_buffer_rd_rid(multimesh);
	rs->get_rendering_device()->buffer_update(instances_buffer, 0, instanced_transforms.size(), instanced_transforms);

	// Set shader parameters
	shader_mat->bind_material_properties(renderer_cache);
	shader_mat->material->set_shader_parameter("in_tex_sim_data", sim_data.get_srv());
	for (uint32_t i_input = 0; i_input < STREAM_OFFSET_MAX; ++i_input) {
		const String name = "in_offset_" + _get_additional_stream_offset_name((AdditionalStreamOffsets)i_input);
		const uint32_t offset = additional_stream_offsets[i_input].offset_for_shader_constant();
		shader_mat->material->set_shader_parameter(name, offset);
	}

	// Request the rendering of our mesh
	if (render_mode == PKRender::RENDER_MODE_3D) {
		Pair<RID, bool> &instance = owner_scene->get_or_create_visual_instance(multimesh);
		rs->instance_set_scenario(instance.first, render_context.scenario_id);
		instance.second = true; // Tell scene that the instance was rendered this frame
	} else {
		Pair<RID, bool> &canvas_item = owner_scene->get_or_create_canvas_item(multimesh);
		rs->canvas_item_set_parent(canvas_item.first, render_context.canvas_id);
		canvas_item.second = true; // Tell scene that the canvas item was rendered this frame
	}

#ifdef PK_DEBUG_AABB
	aabb_debug.update(mesh_bbox);
	aabb_debug.render(owner_scene, render_context.scenario_id);
#endif

	return true;
}

String PKBatchDrawerMeshCPU::_get_additional_stream_offset_name(AdditionalStreamOffsets p_stream) {
	switch (p_stream) {
		case STREAM_OFFSET_COLORS:
			return "colors";
		case STREAM_OFFSET_EMISSIVE_COLORS3:
			return "emissive_colors3";
		case STREAM_OFFSET_EMISSIVE_COLORS4:
			return "emissive_colors4";
		case STREAM_OFFSET_ALPHA_CURSORS:
			return "alpha_cursors";
		case STREAM_OFFSET_ATLAS_TEXTURE_IDS:
			return "texture_ids";
		case STREAM_OFFSET_TRANSFORM_UVS_OFFSETS:
			return "uv_offsets";
		case STREAM_OFFSET_TRANSFORM_UVS_ROTATES:
			return "uv_rotations";
		case STREAM_OFFSET_TRANSFORM_UVS_SCALES:
			return "uv_scales";
		default:
			return "";
	}
}

bool PKBatchDrawerMeshCPU::_is_additional_input_supported(const CStringId &p_input_name, EBaseTypeID p_input_type, AdditionalStreamOffsets &r_stream_offset_type) {
	if (p_input_type == BaseType_Float4) {
		if (p_input_name == BasicRendererProperties::SID_Diffuse_Color() || // Legacy
				p_input_name == BasicRendererProperties::SID_Diffuse_DiffuseColor() ||
				p_input_name == BasicRendererProperties::SID_Distortion_Color()) {
			r_stream_offset_type = STREAM_OFFSET_COLORS;
		} else if (p_input_name == BasicRendererProperties::SID_Emissive_EmissiveColor()) {
			r_stream_offset_type = STREAM_OFFSET_EMISSIVE_COLORS4;
		}
	} else if (p_input_type == BaseType_Float3) {
		if (p_input_name == BasicRendererProperties::SID_Emissive_EmissiveColor()) { // Legacy
			r_stream_offset_type = STREAM_OFFSET_EMISSIVE_COLORS3;
		}
	} else if (p_input_type == BaseType_Float2) {
		if (p_input_name == BasicRendererProperties::SID_TransformUVs_UVOffset()) {
			r_stream_offset_type = STREAM_OFFSET_TRANSFORM_UVS_OFFSETS;
		} else if (p_input_name == BasicRendererProperties::SID_TransformUVs_UVScale()) {
			r_stream_offset_type = STREAM_OFFSET_TRANSFORM_UVS_SCALES;
		}
	} else if (p_input_type == BaseType_Float) {
		if (p_input_name == BasicRendererProperties::SID_AlphaRemap_Cursor() ||
				p_input_name == BasicRendererProperties::SID_AlphaRemap_AlphaRemapCursor()) {
			r_stream_offset_type = STREAM_OFFSET_ALPHA_CURSORS;
		} else if (p_input_name == BasicRendererProperties::SID_Atlas_TextureID()) {
			r_stream_offset_type = STREAM_OFFSET_ATLAS_TEXTURE_IDS;
		} else if (p_input_name == BasicRendererProperties::SID_TransformUVs_UVRotate()) {
			r_stream_offset_type = STREAM_OFFSET_TRANSFORM_UVS_ROTATES;
		}
	}
	return r_stream_offset_type != AdditionalStreamOffsets::STREAM_OFFSET_MAX;
}

void PKBatchDrawerMeshCPU::_clear_stream_offsets() {
	for (u32 i_stream = 0; i_stream < STREAM_OFFSET_MAX; ++i_stream) {
		additional_stream_offsets[i_stream].reset();
	}
}
