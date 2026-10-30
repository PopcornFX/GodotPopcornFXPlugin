//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_2d.h"

#include "godot_cpp/classes/project_settings.hpp"

#include "integration/pk_plugin.h"
#include "integration/pk_plugin_types.h"

namespace godot {

void PKEmitter2D::_update_transforms() {
	if (!is_inside_tree()) {
		return;
	}
	effect_prev_transform = effect_transform;
	Transform2D transform;
	if ((transform_mode == TRANSFORM_DEFAULT && default_transform_mode == TRANSFORM_LOCAL) || transform_mode == TRANSFORM_LOCAL) {
		transform = get_transform();
	} else {
		transform = get_global_transform();
	}
	transform.columns[2] = transform.columns[2] / PKPlugin::get_singleton()->get_project_settings()->unit_pixel_size();
	transform.set_rotation(-transform.get_rotation());
	effect_transform = to_pk(transform);

	effect_prev_velocity = effect_velocity;
	effect_velocity = effect_transform.Translations().xyz() - effect_prev_transform.Translations().xyz();
}

PKScene *PKEmitter2D::_get_pk_scene() {
	return PKPlugin::get_singleton()->get_scene_2d();
}

} // namespace godot
