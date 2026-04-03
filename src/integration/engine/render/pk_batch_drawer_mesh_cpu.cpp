//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_batch_drawer_mesh_cpu.h"

#include "godot_cpp/classes/rendering_server.hpp"

#include "integration/engine/render/pk_frame_collector_types.h"
#include "integration/engine/render/pk_renderer_cache.h"
#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"

#include <pk_maths/include/pk_maths_simd_vector.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

using RS = RenderingServer;

bool PKBatchDrawerMeshCPU::Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) {
	if (!Super::Setup(p_renderer, p_owner, p_fc, p_storage_class)) {
		return false;
	}

	owner_scene = PKPlugin::get_singleton()->get_scene();
	if (owner_scene == nullptr) {
		return false;
	}

	RS *rs = RS::get_singleton();

	// Init mesh and visual instance, then link them together
	multimesh = rs->multimesh_create();
	const RID instance = owner_scene->get_or_create_visual_instance(multimesh).first;
	rs->instance_set_base(instance, multimesh);

	// Get the material and apply its settings to the visual instance
	shader_mat = &owner_scene->get_or_create_shader_material(p_renderer);
	if (shader_mat->mesh == nullptr) {
		return false;
	}
	shader_mat->apply_to_visual_instance(instance);
	rs->multimesh_set_mesh(multimesh, shader_mat->mesh->get_rid());

#ifdef PK_DEBUG_AABB
	aabb_debug.init(owner_scene);
#endif

	sim_data.init();
	return true;
}

void PKBatchDrawerMeshCPU::Destroy() {
	if (owner_scene != nullptr) {
		owner_scene->clear_visual_instance(multimesh);
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
	instanced_transforms.resize(particle_count * sizeof(float) * 12);

	RS::get_singleton()->multimesh_allocate_data(multimesh, particle_count, RenderingServer::MULTIMESH_TRANSFORM_3D);

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
	m_BBJobs_Mesh.m_Exec_Matrices.m_UniformScale = 100;

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
	for (uint32_t i = 0; i < particle_count; ++i) {
		float *ptr = mapped_transforms + i * 12;
		SIMD::Float4 row0, row1, row2, row3;
		SIMD::LoadAligned16_Transpose4x4(row0, row1, row2, row3, &instanced_matrices[i]);
		row0.StoreUnaligned(ptr, 0x00);
		row1.StoreUnaligned(ptr, 0x10);
		row2.StoreUnaligned(ptr, 0x20);
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
	if (!PKGD_VERIFY(renderer_cache != null)) {
		return true;
	}

	RS *rs = RS::get_singleton();

	// Set the bounding box of the batch drawer for culling
	const AABB mesh_bbox = to_gd(p_to_emit.m_DrawRequests[0]->BBox());
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
	Pair<RID, bool> &instance = owner_scene->get_or_create_visual_instance(multimesh);
	RS::get_singleton()->instance_set_scenario(instance.first, render_context.scenario_id);
	instance.second = true; // Tell scene that the instance was rendered this frame

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
		if (p_input_name == BasicRendererProperties::SID_AlphaRemap_Cursor()) {
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
