//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/multi_mesh.hpp"
#include "godot_cpp/variant/rid.hpp"

#include "core/pk_buffer.h"
#include "core/pk_shader_material.h"
#include "integration/engine/render/pk_render_type_policies.h"
#include "integration/engine/render/pk_render_utils.h"

#include <pk_render_helpers/include/batch_jobs/rh_batch_jobs_mesh_cpu.h>

using namespace godot;

class PKScene;

class PKBatchDrawerMeshCPU : public CRendererBatchJobs_Mesh_CPUBB {
public:
	typedef CRendererBatchJobs_Mesh_CPUBB Super;

	PKBatchDrawerMeshCPU() = default;
	~PKBatchDrawerMeshCPU() = default;

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
		STREAM_OFFSET_ALPHA_CURSORS,
		STREAM_OFFSET_ATLAS_TEXTURE_IDS,
		STREAM_OFFSET_TRANSFORM_UVS_OFFSETS,
		STREAM_OFFSET_TRANSFORM_UVS_ROTATES,
		STREAM_OFFSET_TRANSFORM_UVS_SCALES,
		STREAM_OFFSET_MAX
	};

	// Returns the name of the given additional stream offset
	static String _get_additional_stream_offset_name(AdditionalStreamOffsets p_stream);

	static bool _is_additional_input_supported(const CStringId &p_input_name, EBaseTypeID p_input_type, AdditionalStreamOffsets &r_stream_offset_type);
	void _clear_stream_offsets();

	uint32_t particle_count = 0; // Total number of billboarded particles

	PKScene *owner_scene = nullptr;
	const PKShaderMaterial *shader_mat = nullptr;
	RID multimesh;

#ifdef PK_DEBUG_AABB
	PKRenderDebugAABB aabb_debug;
#endif

	TArray<CFloat4x4, TArrayAligned16> instanced_matrices;
	PackedByteArray instanced_transforms;

	PKBuffer sim_data;
	TArray<PKAdditionalInput> additional_inputs;
	TArray<Drawers::SCopyFieldDesc> additional_inputs_mapped;
	TStaticArray<PKStreamOffset, STREAM_OFFSET_MAX> additional_stream_offsets;
};
