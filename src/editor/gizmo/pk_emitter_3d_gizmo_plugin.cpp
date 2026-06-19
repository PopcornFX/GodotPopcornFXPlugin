#include "pk_emitter_3d_gizmo_plugin.h"

#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/standard_material3d.hpp"

#include "scene/pk_emitter_3d.h"

namespace godot {

PKEmitter3DGizmoPlugin::PKEmitter3DGizmoPlugin() {
	loaded = false;
}

bool PKEmitter3DGizmoPlugin::_has_gizmo(Node3D *p_for_node_3d) const {
	return cast_to<PKEmitter3D>(p_for_node_3d) != nullptr;
}

void PKEmitter3DGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo> &p_gizmo) {
	if (!_try_load()) {
		return;
	}
	p_gizmo->clear();
	const Ref<Material> icon = get_material("pk_emitter_3d_icon", p_gizmo);
	p_gizmo->add_unscaled_billboard(icon, 0.05);
}

// The icon can be unmounted when we're first being constructed and it hasn't been imported before
bool PKEmitter3DGizmoPlugin::_try_load() {
	if (loaded) {
		return true;
	}
	ResourceLoader *loader = ResourceLoader::get_singleton();
	if (loader->exists("res://addons/popcornfx/icons/PKEmitter3DGizmo.svg")) {
		const Ref<Texture2D> icon = loader->load("res://addons/popcornfx/icons/PKEmitter3DGizmo.svg");
		if (icon.is_null()) {
			return false;
		}
		create_icon_material("pk_emitter_3d_icon", icon);
		loaded = true;
	}
	return loaded;
}

} //namespace godot
