//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_render_utils.h"

#include "godot_cpp/classes/rendering_server.hpp"

#ifdef PK_DEBUG_AABB
#include "godot_cpp/classes/random_number_generator.hpp"
#endif

using RS = RenderingServer;

uint64_t apply_mesh_format_version(uint64_t p_format) {
	p_format &= ~(RS::ARRAY_FLAG_FORMAT_VERSION_MASK << RS::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
	p_format |= RS::ARRAY_FLAG_FORMAT_CURRENT_VERSION & (RS::ARRAY_FLAG_FORMAT_VERSION_MASK << RS::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
	return p_format;
}

#ifdef PK_DEBUG_AABB
void PKRenderDebugAABB::init(PKScene *p_owner_scene) {
	RandomNumberGenerator *rng = memnew(RandomNumberGenerator);
	color.set_hsv(rng->randf() * Math_PI * 2, 1, 1, 1);
	memdelete(rng);

	mat.instantiate();
	mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

	mesh.instantiate();
	surf_tool.instantiate();

	RS *rs = RS::get_singleton();
	const RID instance = p_owner_scene->get_or_create_visual_instance(mesh->get_rid()).first;
	rs->instance_set_base(instance, mesh->get_rid());
	rs->instance_geometry_set_cast_shadows_setting(instance, RS::SHADOW_CASTING_SETTING_OFF);
}

void PKRenderDebugAABB::destroy(PKScene *p_owner_scene) {
	p_owner_scene->clear_visual_instance(mesh->get_rid());
}

void PKRenderDebugAABB::update(const AABB &p_aabb) {
	mesh->clear_surfaces();

	surf_tool->begin(ArrayMesh::PRIMITIVE_LINES);

	surf_tool->set_material(mat);

	for (int i = 0; i < 8; i++) {
		surf_tool->set_color(color);
		surf_tool->add_vertex(p_aabb.get_endpoint(i));
	}

	surf_tool->add_index(0);
	surf_tool->add_index(1);

	surf_tool->add_index(0);
	surf_tool->add_index(2);

	surf_tool->add_index(2);
	surf_tool->add_index(3);

	surf_tool->add_index(1);
	surf_tool->add_index(3);

	surf_tool->add_index(4);
	surf_tool->add_index(5);

	surf_tool->add_index(4);
	surf_tool->add_index(6);

	surf_tool->add_index(6);
	surf_tool->add_index(7);

	surf_tool->add_index(5);
	surf_tool->add_index(7);

	surf_tool->add_index(0);
	surf_tool->add_index(4);

	surf_tool->add_index(1);
	surf_tool->add_index(5);

	surf_tool->add_index(2);
	surf_tool->add_index(6);

	surf_tool->add_index(3);
	surf_tool->add_index(7);

	surf_tool->commit(mesh);
}

void PKRenderDebugAABB::render(PKScene *p_owner_scene, RID p_scenario_id) {
	Pair<RID, bool> &instance = p_owner_scene->get_or_create_visual_instance(mesh->get_rid());
	RS::get_singleton()->instance_set_scenario(instance.first, p_scenario_id);
	instance.second = true; // Tell scene that the instance was rendered this frame
}
#endif
