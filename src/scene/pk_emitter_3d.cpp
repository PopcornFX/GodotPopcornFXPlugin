//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_3d.h"

#include "integration/pk_plugin.h"
#include "integration/pk_plugin_types.h"

namespace godot {

void PKEmitter3D::_update_transforms() {
	if (!is_inside_tree()) {
		return;
	}
	effect_prev_transform = effect_transform;
	if ((transform_mode == TRANSFORM_DEFAULT && default_transform_mode == TRANSFORM_LOCAL) || transform_mode == TRANSFORM_LOCAL) {
		effect_transform = to_pk(get_transform());
	} else {
		effect_transform = to_pk(get_global_transform());
	}
	effect_prev_velocity = effect_velocity;
	effect_velocity = effect_transform.Translations().xyz() - effect_prev_transform.Translations().xyz();
}

PKScene *PKEmitter3D::_get_pk_scene() {
	return PKPlugin::get_singleton()->get_scene_3d();
}
} // namespace godot
