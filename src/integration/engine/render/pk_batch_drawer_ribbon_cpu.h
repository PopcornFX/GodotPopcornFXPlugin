//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/array_mesh.hpp"

#include "core/pk_buffer.h"
#include "core/pk_packed_byte_array.h"
#include "core/pk_shader_material.h"
#include "integration/pk_error_handling.h"
#include "pk_render_type_policies.h"
#include "pk_render_utils.h"

#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_ribbon_cpu.h>
#include <pk_render_helpers/include/draw_requests/rh_draw_requests.h>

using namespace godot;

class PKScene;

class PKBatchDrawerRibbonCPU : public CRendererBatchJobs_Ribbon_CPUBB {
public:
	typedef CRendererBatchJobs_Ribbon_CPUBB Super;

	PKBatchDrawerRibbonCPU() = default;
	~PKBatchDrawerRibbonCPU() = default;

	virtual bool Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) override;
	virtual void Destroy() override;
	virtual bool AreRenderersCompatible(const CRendererDataBase *p_renderer_a, const CRendererDataBase *p_renderer_b) const override;

	virtual bool AllocBuffers(SRenderContext &p_ctx) override;
	virtual bool MapBuffers(SRenderContext &p_ctx) override;
	virtual bool UnmapBuffers(SRenderContext &p_ctx) override;
	virtual bool EmitDrawCall(SRenderContext &p_ctx, const SDrawCallDesc &p_to_emit) override;

private:
	enum AdditionalStreamOffsets {
		STREAM_OFFSET_COLORS = 0,
		STREAM_OFFSET_EMISSIVE_COLORS3,
		STREAM_OFFSET_EMISSIVE_COLORS4,
		// TODO
		//STREAM_OFFSET_VELOCITY,
		STREAM_OFFSET_ALPHA_CURSORS,
		STREAM_OFFSET_ATLAS_TEXTURE_IDS,
		STREAM_OFFSET_TRANSFORM_UVS_OFFSETS,
		STREAM_OFFSET_TRANSFORM_UVS_ROTATES,
		STREAM_OFFSET_TRANSFORM_UVS_SCALES,
		STREAM_OFFSET_MAX
	};

	// Returns the name of the given additional stream offset
	static String _get_additional_stream_offset_name(AdditionalStreamOffsets p_stream);

	bool _should_gen_input(Drawers::EGeneratedInput p_input) const;
	bool _should_gen_input(Drawers::EGeneratedInput p_input, uint32_t p_view_idx) const;

	static bool _is_additional_input_supported(const CStringId &p_input_name, EBaseTypeID p_input_type, AdditionalStreamOffsets &r_stream_offset_type);
	void _clear_stream_offsets();

	const uint32_t mesh_buffers_padding = 1024;

	uint32_t particle_count = 0; // Total number of billboarded particles
	uint32_t idx_count = 0; // Total number of billboarded indices
	uint32_t vtx_count = 0; // Total number of billboarded vertices
	uint32_t idx_count_alloc = 0; // Total number of indices allocated in the index buffer
	uint32_t vtx_count_alloc = 0; // Total number of vertices allocated in the vertex buffer

	uint64_t mesh_format = 0;
	uint32_t idx_stride = 0; // Size of an index
	uint32_t atb_stride = 0; // Size of an attribute (texture coords)
	uint32_t vtx_stride = 0; // Size of a vertex
	uint32_t uv_factors_stride = 0; // Size of a uv factors
	uint32_t nrm_tan_stride = 0; // Size of a normal and a tangent combined
	uint32_t vnt_stride = 0; // Size of a vertex, a normal and a tangent combined

	bool large_indices = false;
	bool realloc_surface = false; // Reset every frame, used to choose whether to update surface data or realloc it entirely

	bool has_correct_deformation = false;
	bool has_rotate_uv = false;
	bool has_second_uv_set = false;

	uint8_t vpp = 0;

	PKScene *owner_scene = nullptr;
	const PKShaderMaterial *shader_mat = nullptr;
	Ref<ArrayMesh> mesh;
	Dictionary surface;

#ifdef PK_DEBUG_AABB
	PKRenderDebugAABB aabb_debug;
#endif

	PKPackedByteArray idx_array; // CPU index array
	PKPackedByteArray atb_array; // CPU attribute array (texture coords)
	PKPackedByteArray vtx_array; // CPU vertex array (contains packed normals/tangents) with Godot format (float 3, unaligned)
	PKPackedByteArray uv_factors_array; // CPU UV factors array
	TArray<CFloat3, TArrayAligned16> vnt_array; // CPU Vertex (float3, 16bytes-aligned), normals (float3, 16bytes-aligned) and tangent (float4) array
	//PKPackedByteArray vnt_array; // CPU Vertex (float3, 16bytes-aligned), normals (float3, 16bytes-aligned) and tangent (float4) array

	PKBuffer sim_data;
	TArray<PKAdditionalInput> additional_inputs;
	TArray<Drawers::SCopyFieldDesc> additional_inputs_mapped;
	TStaticArray<PKStreamOffset, STREAM_OFFSET_MAX> additional_stream_offsets;
};
