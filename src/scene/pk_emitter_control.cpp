//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_control.h"

#include "godot_cpp/classes/project_settings.hpp"

#include "integration/internal/pk_project_settings.h"
#include "integration/pk_plugin.h"
#include "integration/pk_plugin_types.h"

namespace godot {
PKEmitterControl::PKEmitterControl() {
	set_mouse_filter(MOUSE_FILTER_IGNORE);
}

void PKEmitterControl::_update_transforms() {
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
	Vector2 size = get_size();
	transform.translate_local(size.x / 2, size.y / 2);
	transform.columns[2] = transform.columns[2] / PKPlugin::get_singleton()->get_project_settings()->unit_pixel_size();
	transform.set_rotation(-transform.get_rotation());
	effect_transform = to_pk(transform);
	effect_prev_velocity = effect_velocity;
	effect_velocity = effect_transform.Translations().xyz() - effect_prev_transform.Translations().xyz();
}

PKScene *PKEmitterControl::_get_pk_scene() {
	return PKPlugin::get_singleton()->get_scene_2d();
}

} // namespace godot
