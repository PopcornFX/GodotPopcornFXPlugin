//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_batch_drawer_ribbon_cpu.h"

#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/shader_material.hpp"

#include "integration/engine/render/pk_frame_collector_types.h"
#include "integration/engine/render/pk_render_utils.h"
#include "integration/engine/render/pk_renderer_cache.h"
#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"
#include "integration/pk_plugin_types.h"

#include <pk_maths/include/pk_maths_simd_vector.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

using RS = RenderingServer;

bool PKBatchDrawerRibbonCPU::Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) {
	if (!Super::Setup(p_renderer, p_owner, p_fc, p_storage_class)) {
		return false;
	}

	owner_scene = PKPlugin::get_singleton()->get_scene();
	if (owner_scene == nullptr) {
		return false;
	}

	// Init mesh and visual instance, then link them together
	mesh.instantiate();
	const RID instance = owner_scene->get_or_create_visual_instance(mesh->get_rid()).first;
	RS::get_singleton()->instance_set_base(instance, mesh->get_rid());

	// Get the material and apply its settings to the visual instance
	shader_mat = &owner_scene->get_or_create_shader_material(p_renderer);
	shader_mat->apply_to_visual_instance(instance);

	// Init constant fields of the mesh surface dict
	surface["primitive"] = ArrayMesh::PRIMITIVE_TRIANGLES;
	surface["material"] = shader_mat->material->get_rid();
	surface["uv_scale"] = Vector4(0.0, 0.0, 0.0, 0.0);

	const PRendererCacheBase cache = p_renderer->m_RendererCache;
	has_correct_deformation = cache->m_Flags.m_HasRibbonCorrectDeformation;
	has_second_uv_set = cache->m_Flags.m_HasAtlasBlending && cache->m_Flags.m_HasUV;
	has_rotate_uv = cache->m_Flags.m_RotateTexture;

	const PopcornFX::ERibbonMode mode = p_renderer->m_Declaration.GetPropertyValue_Enum<ERibbonMode>(BasicRendererProperties::SID_BillboardingMode(), PopcornFX::RibbonMode_ViewposAligned);
	if (mode == PopcornFX::RibbonMode_SideAxisAlignedTube) {
		vpp = (p_renderer->m_Declaration.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_GeometryRibbon_SegmentCount(), 8) + 1) * 2;
	} else if (mode == PopcornFX::RibbonMode_SideAxisAlignedMultiPlane) {
		vpp = p_renderer->m_Declaration.GetPropertyValue_I1(PopcornFX::BasicRendererProperties::SID_GeometryRibbon_PlaneCount(), 2) * 4;
	}

#ifdef PK_DEBUG_AABB
	aabb_debug.init(owner_scene);
#endif

	sim_data.init();
	return true;
}

void PKBatchDrawerRibbonCPU::Destroy() {
	if (owner_scene != nullptr) {
		owner_scene->clear_visual_instance(mesh->get_rid());
#ifdef PK_DEBUG_AABB
		aabb_debug.destroy(owner_scene);
#endif
	}

	sim_data.destroy();
}

bool PKBatchDrawerRibbonCPU::AreRenderersCompatible(const CRendererDataBase *p_renderer_a, const CRendererDataBase *p_renderer_b) const {
	if (!Super::AreRenderersCompatible(p_renderer_a, p_renderer_b)) {
		return false;
	}

	const PKShaderMaterial &shader_a = owner_scene->get_or_create_shader_material(p_renderer_a);
	const PKShaderMaterial &shader_b = owner_scene->get_or_create_shader_material(p_renderer_b);
	return shader_a == shader_b;
}

bool PKBatchDrawerRibbonCPU::AllocBuffers(SRenderContext &p_ctx) {
	if (!PKGD_VERIFY(m_DrawPass != nullptr)) {
		return false;
	}

	PKGD_ASSERT(m_DrawPass->m_TotalParticleCount == m_BB_Ribbon.TotalParticleCount());
	particle_count = m_DrawPass->m_TotalParticleCount;
	vtx_count = m_BB_Ribbon.TotalVertexCount();
	idx_count = m_BB_Ribbon.TotalIndexCount();
	const RS *rs = RS::get_singleton();

	const uint64_t prev_mesh_format = mesh_format;
	mesh_format = 0;

	// -----------------------------------------
	// View independent inputs:
	// Data required for particle rendering that does not vary per view
	// -----------------------------------------
	if (_should_gen_input(Drawers::GenInput_Indices)) {
		mesh_format |= RS::ARRAY_FORMAT_INDEX;
	}
	if (_should_gen_input(Drawers::GenInput_Position)) {
		mesh_format |= RS::ARRAY_FORMAT_VERTEX;
	}
	if (_should_gen_input(Drawers::GenInput_Normal)) {
		mesh_format |= RS::ARRAY_FORMAT_NORMAL;
	}
	if (_should_gen_input(Drawers::GenInput_Tangent)) {
		mesh_format |= RS::ARRAY_FORMAT_TANGENT;
	}
	// TODO: UV factors
	/*if (should_gen_input(Drawers::GenInput_UVFactors)) {
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV;
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV2;
	}*/
	if (_should_gen_input(Drawers::GenInput_UV0)) {
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV;
	}
	if (_should_gen_input(Drawers::GenInput_UV1)) {
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV2;
	}

	// -----------------------------------------
	// View dependent inputs:
	// Data required for particle rendering that varies per view (for example, view aligned quads have view dependent positions)
	// Note: the difference between view dependent and view independent data is an optimization to avoid re-generating the same data for each view when not necessary.
	// -----------------------------------------
	const uint32_t view_count = m_DrawPass->m_ToGenerate.m_PerViewGeneratedInputs.Count();
	if (view_count > 1) {
		return false;
	}
	if (view_count == 0) {
		return true;
	}

	// If indices are view dependent, it means particles need to be sorted per view
	if (_should_gen_input(Drawers::GenInput_Indices, 0)) {
		mesh_format |= RS::ARRAY_FORMAT_INDEX;
	}
	// If positions/normals/tangents are view dependent, it means the particles have a BillboardingMode different from PlaneAligned: its geometry changes per view
	if (_should_gen_input(Drawers::GenInput_Position, 0)) {
		mesh_format |= RS::ARRAY_FORMAT_VERTEX;
	}
	if (_should_gen_input(Drawers::GenInput_Normal, 0)) {
		mesh_format |= RS::ARRAY_FORMAT_NORMAL;
	}
	if (_should_gen_input(Drawers::GenInput_Tangent, 0)) {
		mesh_format |= RS::ARRAY_FORMAT_TANGENT;
	}
	// TODO: UV factors
	/*if (should_gen_input(Drawers::GenInput_UVFactors, 0)) {
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV;
		mesh_format |= RS::ARRAY_FORMAT_TEX_UV2;
	}*/

	// Allocate more elements than necessary to realloc less often
	// Indices are aligned on 3 because using PRIMITIVE_TRIANGLES requires 3 elements per primitive
	const uint32_t idx_count_padded = Mem::Align(Mem::Align(idx_count, mesh_buffers_padding), 3);
	const uint32_t vtx_count_padded = Mem::Align(vtx_count, mesh_buffers_padding);

	realloc_surface = (mesh_format != prev_mesh_format) ||
			(idx_count_alloc != idx_count_padded) ||
			(vtx_count_alloc != vtx_count_padded);

	if (realloc_surface) {
		idx_count_alloc = idx_count_padded;
		vtx_count_alloc = vtx_count_padded;

		// Get element strides (done for every frame since it varies based on vertex count)
		idx_stride = rs->mesh_surface_get_format_index_stride(mesh_format, vtx_count_alloc);
		atb_stride = rs->mesh_surface_get_format_attribute_stride(mesh_format, vtx_count_alloc);
		vtx_stride = rs->mesh_surface_get_format_vertex_stride(mesh_format, vtx_count_alloc);
		nrm_tan_stride = rs->mesh_surface_get_format_normal_tangent_stride(mesh_format, vtx_count_alloc);
		vnt_stride = vtx_stride + nrm_tan_stride;
		large_indices = (idx_stride == sizeof(uint32_t));

		// Resize index, UV0/UV1, and vertex/normal/tangent arrays
		idx_array.resize(idx_count_alloc * idx_stride);
		atb_array.resize(vtx_count_alloc * atb_stride);
		vtx_array.resize(vtx_count_alloc * vnt_stride);
		// TODO: UV factors
		//uv_factors_array.resize(vtx_count_alloc * atb_stride);
		if (nrm_tan_stride > 0) {
			vnt_array.Resize(vtx_count_alloc * sizeof(CFloat4) * 3);
		} else {
			vnt_array.Resize(vtx_count_alloc * sizeof(CFloat4));
		}
	}

	// -----------------------------------------
	// Additional inputs:
	// Per-particle data sent to shaders (AlphaRemapCursor, Colors, ..)
	// -----------------------------------------
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

bool PKBatchDrawerRibbonCPU::MapBuffers(SRenderContext &p_ctx) {
	(void)p_ctx;
	PKGD_ASSERT(m_DrawPass != nullptr);

	// -----------------------------------------
	// View independent
	// -----------------------------------------
	if (_should_gen_input(Drawers::GenInput_Indices)) {
		m_BBJobs_Ribbon.m_Exec_Indices.m_IndexStream.Setup(idx_array.ptrw(), idx_count, large_indices);
	}
	if (_should_gen_input(Drawers::GenInput_Position)) {
		void *positions_mapped = vnt_array.RawDataPointer();
		TStridedMemoryView<CFloat3, 0x10> positions_view(static_cast<CFloat3 *>(positions_mapped), vtx_count, 0x10);
		m_BBJobs_Ribbon.m_Exec_PNT.m_Positions = positions_view;
	}
	if (_should_gen_input(Drawers::GenInput_Normal)) {
		void *normals_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc;
		TStridedMemoryView<CFloat3, 0x10> normals_view(static_cast<CFloat3 *>(normals_mapped), vtx_count, 0x10);
		m_BBJobs_Ribbon.m_Exec_PNT.m_Normals = normals_view;
	}
	if (_should_gen_input(Drawers::GenInput_Tangent)) {
		void *tangents_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc * 2;
		TStridedMemoryView<CFloat4, 0x10> tangents_view(static_cast<CFloat4 *>(tangents_mapped), vtx_count, 0x10);
		m_BBJobs_Ribbon.m_Exec_PNT.m_Tangents = tangents_view;
	}
	// TODO: UV factors
	//if (should_gen_input(Drawers::GenInput_UVFactors)) {
	//	void *uv_factors_mapped = uv_factors_array.ptrw() + sizeof(CFloat4) * vtx_count_alloc;
	//	TStridedMemoryView<CFloat4> uv_factors_view(static_cast<CFloat4 *>(uv_factors_mapped), vtx_count);
	//	m_BBJobs_Ribbon.m_Exec_PNT.m_UVFactors4 = uv_factors_view;
	//}
	if (_should_gen_input(Drawers::GenInput_UV0)) {
		void *texcoords_mapped = atb_array.ptrw();
		TStridedMemoryView<CFloat2> texcoords_view(static_cast<CFloat2 *>(texcoords_mapped), vtx_count, atb_stride);
		m_BBJobs_Ribbon.m_Exec_Texcoords.m_Texcoords = texcoords_view;
	}
	if (_should_gen_input(Drawers::GenInput_UV1)) {
		void *texcoords2_mapped = atb_array.ptrw() + sizeof(CFloat2);
		TStridedMemoryView<CFloat2> texcoords2_view(static_cast<CFloat2 *>(texcoords2_mapped), vtx_count, atb_stride);
		m_BBJobs_Ribbon.m_Exec_Texcoords.m_Texcoords2 = texcoords2_view;
	}

	// -----------------------------------------
	// View dependent
	// -----------------------------------------
	const uint32_t view_count = m_DrawPass->m_ToGenerate.m_PerViewGeneratedInputs.Count();
	if (view_count > 1) {
		return false;
	}
	if (view_count == 0) {
		return true;
	}

	SRibbonBatchJobs::SPerView &cur_view_jobs = m_BBJobs_Ribbon.m_PerView[0];
	if (_should_gen_input(Drawers::GenInput_Indices, 0)) {
		cur_view_jobs.m_Exec_Indices.m_IndexStream.Setup(idx_array.ptrw(), idx_count, large_indices);
	}
	if (_should_gen_input(Drawers::GenInput_Position, 0)) {
		void *positions_mapped = vnt_array.RawDataPointer();
		TStridedMemoryView<CFloat3, 0x10> positions_view(static_cast<CFloat3 *>(positions_mapped), vtx_count, 0x10);
		cur_view_jobs.m_Exec_PNT.m_Positions = positions_view;
	}
	if (_should_gen_input(Drawers::GenInput_Normal, 0)) {
		void *normals_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc;
		TStridedMemoryView<CFloat3, 0x10> normals_view(static_cast<CFloat3 *>(normals_mapped), vtx_count, 0x10);
		cur_view_jobs.m_Exec_PNT.m_Normals = normals_view;
	}
	if (_should_gen_input(Drawers::GenInput_Tangent, 0)) {
		void *tangents_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc * 2;
		TStridedMemoryView<CFloat4, 0x10> tangents_view(static_cast<CFloat4 *>(tangents_mapped), vtx_count, 0x10);
		cur_view_jobs.m_Exec_PNT.m_Tangents = tangents_view;
	}
	// TODO: UV factors
	//if (should_gen_input(Drawers::GenInput_UVFactors)) {
	//	void *uv_factors_mapped = uv_factors_array.ptrw() + sizeof(CFloat4) * vtx_count_alloc;
	//	TStridedMemoryView<CFloat4> uv_factors_view(static_cast<CFloat4 *>(uv_factors_mapped), vtx_count);
	//	cur_view_jobs.m_Exec_PNT.m_UVFactors4 = uv_factors_view;
	//}

	// -----------------------------------------
	// Additional inputs
	// -----------------------------------------
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

		m_BBJobs_Ribbon.m_Exec_CopyField.m_FieldsToCopy = additional_inputs_mapped.View();
		m_BBJobs_Ribbon.m_Exec_CopyField.m_PerVertex = false;
	}
	return true;
}

bool PKBatchDrawerRibbonCPU::UnmapBuffers(SRenderContext &p_ctx) {
	// Copying all the vertices at the beginning of the vertex buffer with the right format... (float3)
	void *pos_mapped = vnt_array.RawDataPointer();
	TStridedMemoryView<CFloat3, 0x10> pos_view(static_cast<CFloat3 *>(pos_mapped), vtx_count, 0x10);
	float *mapped_transforms = reinterpret_cast<float *>(vtx_array.ptrw());
	for (uint32_t i = 0; i < vtx_count; ++i) {
		float *ptr = mapped_transforms + i * 3;
		SIMD::Float4 row0 = SIMD::Float4::LoadAligned16(&pos_view[i], 0x00);
		row0.StoreUnaligned(ptr, 0x00);
	}

	// Write normals and tangents at the end of the vertex buffer
	if (nrm_tan_stride > 0) {
		const uint32_t write_offset = vtx_count_alloc * vtx_stride;
		const uint32_t tangent_offset = vtx_count_alloc * sizeof(CFloat4);
		void *normals_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc;
		TStridedMemoryView<CFloat3, 0x10> normal_view(static_cast<CFloat3 *>(normals_mapped), vtx_count, 0x10);
		void *tangents_mapped = vnt_array.RawDataPointer() + sizeof(CFloat4) * vtx_count_alloc * 2;
		TStridedMemoryView<CFloat4, 0x10> tan_view(static_cast<CFloat4 *>(tangents_mapped), vtx_count, 0x10);

		// Same packing as in RenderingServer::_surface_set_data
		for (uint32_t i = 0; i < vtx_count; ++i) {
			const uint32_t read_offset = i;

			// Not sure why normals need to be flipped...
			const CFloat3 normal = -normal_view[read_offset];
			const CFloat3 tangent = tan_view[read_offset].xyz();
			const float tan_sign = tan_view[read_offset].w();

			const Vector3 gd_normal(normal.x(), normal.y(), normal.z());
			const Vector3 gd_tangent(tangent.x(), tangent.y(), tangent.z());

			const Vector2 normal_encoded = gd_normal.octahedron_encode();
			const Vector2 tangent_encoded = gd_tangent.octahedron_tangent_encode(tan_sign);

			uint16_t packed[4] = {
				(uint16_t)CLAMP(normal_encoded.x * 65535, 0, 65535),
				(uint16_t)CLAMP(normal_encoded.y * 65535, 0, 65535),
				(uint16_t)CLAMP(tangent_encoded.x * 65535, 0, 65535),
				(uint16_t)CLAMP(tangent_encoded.y * 65535, 0, 65535),
			};

			// Necessary to sanitize tangents here:
			// (1, 1) and (0, 1) decode to the same value, but (0, 1) messes with godot's compression detection
			if (packed[2] == 0 && packed[3] == 65535) {
				packed[2] = 65535;
			}

			memcpy(&vtx_array[write_offset + i * nrm_tan_stride], packed, 8);
		}
	}

	// Clear unused indices at the end of the index buffer
	idx_array.clear_slack_data(idx_count * idx_stride);

	sim_data.unmap();
	return true;
}

bool PKBatchDrawerRibbonCPU::EmitDrawCall(SRenderContext &p_ctx, const SDrawCallDesc &p_to_emit) {
	if (owner_scene == nullptr) {
		return false;
	}

	const PKRenderContext &render_context = static_cast<PKRenderContext &>(p_ctx);
	const PKRendererCache *renderer_cache = static_cast<PKRendererCache *>(p_to_emit.m_RendererCaches.First().Get());
	if (!PKGD_VERIFY(renderer_cache != null)) {
		return true;
	}

	RS *rs = RS::get_singleton();
	const RID mesh_rid = mesh->get_rid();
	const AABB mesh_bbox = to_gd(p_to_emit.m_BBox);

	// Realloc surface if buffer sizes changed, otherwise update data
	if (realloc_surface) {
		surface["format"] = apply_mesh_format_version(mesh_format);
		surface["index_count"] = idx_count_alloc;
		surface["vertex_count"] = vtx_count_alloc;
		surface["index_data"] = idx_array;
		surface["attribute_data"] = atb_array;
		surface["vertex_data"] = vtx_array;
		// WIP: UV factors
		//surface["uv_scale"] = uv_factors_array;
		surface["aabb"] = mesh_bbox;

		mesh->clear_surfaces();
		rs->mesh_add_surface(mesh_rid, surface);
	} else {
		const bool has_views = !m_DrawPass->m_Views.Empty();

		const bool update_indices = _should_gen_input(Drawers::GenInput_Indices) || (has_views && _should_gen_input(Drawers::GenInput_Indices, 0));

		const bool update_vertices =
				_should_gen_input(Drawers::GenInput_Position) || (has_views && _should_gen_input(Drawers::GenInput_Position, 0)) ||
				_should_gen_input(Drawers::GenInput_Normal) || (has_views && _should_gen_input(Drawers::GenInput_Normal, 0)) ||
				_should_gen_input(Drawers::GenInput_Tangent) || (has_views && _should_gen_input(Drawers::GenInput_Tangent, 0));

		const bool update_attributes = _should_gen_input(Drawers::GenInput_UV0) || _should_gen_input(Drawers::GenInput_UV1);

		if (update_indices) {
			rs->mesh_surface_update_index_region(mesh_rid, 0, 0, idx_array);
		}
		if (update_vertices) {
			rs->mesh_surface_update_vertex_region(mesh_rid, 0, 0, vtx_array);
		}
		if (update_attributes) {
			rs->mesh_surface_update_attribute_region(mesh_rid, 0, 0, atb_array);
		}
	}
	mesh->set_custom_aabb(mesh_bbox);

	// Set shader parameters
	shader_mat->bind_material_properties(renderer_cache);
	shader_mat->material->set_shader_parameter("in_total_particle_count", vtx_count);
	shader_mat->material->set_shader_parameter("in_vpp", vpp);
	shader_mat->material->set_shader_parameter("in_tubes_planes_offset", vpp > 0 ? 0 : p_to_emit.m_TotalParticleCount);
	shader_mat->material->set_shader_parameter("in_tex_sim_data", sim_data.get_srv());
	for (uint32_t i_input = 0; i_input < STREAM_OFFSET_MAX; ++i_input) {
		const String name = "in_offset_" + _get_additional_stream_offset_name((AdditionalStreamOffsets)i_input);
		const uint32_t offset = additional_stream_offsets[i_input].offset_for_shader_constant();
		shader_mat->material->set_shader_parameter(name, offset);
	}

	// Request the rendering of our mesh
	Pair<RID, bool> &instance = owner_scene->get_or_create_visual_instance(mesh->get_rid());
	rs->instance_set_scenario(instance.first, render_context.scenario_id);
	instance.second = true; // Tell scene that the instance was rendered this frame

#ifdef PK_DEBUG_AABB
	aabb_debug.update(mesh_bbox);
	aabb_debug.render(owner_scene, render_context.scenario_id);
#endif
	return true;
}

String PKBatchDrawerRibbonCPU::_get_additional_stream_offset_name(AdditionalStreamOffsets p_stream) {
	switch (p_stream) {
		case STREAM_OFFSET_COLORS:
			return "colors";
		case STREAM_OFFSET_EMISSIVE_COLORS3:
			return "emissive_colors3";
		case STREAM_OFFSET_EMISSIVE_COLORS4:
			return "emissive_colors4";
		// TODO
		//case STREAM_OFFSET_VELOCITY:
		//	return "velocity";
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

bool PKBatchDrawerRibbonCPU::_should_gen_input(Drawers::EGeneratedInput p_input) const {
	PKGD_ASSERT(m_DrawPass != nullptr);
	return m_DrawPass->m_ToGenerate.m_GeneratedInputs & p_input;
}

bool PKBatchDrawerRibbonCPU::_should_gen_input(Drawers::EGeneratedInput p_input, uint32_t p_view_idx) const {
	PKGD_ASSERT(m_DrawPass != nullptr);
	PKGD_ASSERT(p_view_idx < m_DrawPass->m_ToGenerate.m_PerViewGeneratedInputs.Count());
	return m_DrawPass->m_ToGenerate.m_PerViewGeneratedInputs[p_view_idx].m_GeneratedInputs & p_input;
}

bool PKBatchDrawerRibbonCPU::_is_additional_input_supported(const CStringId &p_input_name, EBaseTypeID p_input_type, AdditionalStreamOffsets &r_stream_offset_type) {
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
		// TODO
		//} else if (p_input_name == BasicRendererProperties::SID_ComputeVelocity_PreviousPosition()) {
		//	r_stream_offset_type = STREAM_OFFSET_VELOCITY;
		//}
	}
	return r_stream_offset_type != AdditionalStreamOffsets::STREAM_OFFSET_MAX;
}

void PKBatchDrawerRibbonCPU::_clear_stream_offsets() {
	for (u32 i_stream = 0; i_stream < STREAM_OFFSET_MAX; ++i_stream) {
		additional_stream_offsets[i_stream].reset();
	}
}
