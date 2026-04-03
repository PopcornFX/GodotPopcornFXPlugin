//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"
#include "godot_cpp/classes/surface_tool.hpp"

#include "integration/internal/pk_scene.h"

using namespace godot;

// Helper functions for rendering

// Force mesh format version to the current version as done in RenderingServer::mesh_create_surface_data_from_arrays
uint64_t apply_mesh_format_version(uint64_t p_format);

#if 0
#define PK_DEBUG_AABB
struct PKRenderDebugAABB {
	Color color;
	Ref<ArrayMesh> mesh;
	Ref<SurfaceTool> surf_tool;
	Ref<StandardMaterial3D> mat;

	void init(PKScene *p_owner_scene);
	void destroy(PKScene *p_owner_scene);
	void update(const AABB &p_aabb);
	void render(PKScene *p_owner_scene, RID p_scenario_id);
};
#endif
