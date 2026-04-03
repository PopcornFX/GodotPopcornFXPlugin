//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_inspector_plugin.h"

#include "pk_emitter_properties_editor.h"
#include "scene/pk_emitter_3d.h"

namespace godot {
void PKInspectorPlugin::_parse_category(Object *p_object, const String &p_category) {
	if (p_category != "PKEmitter3D") {
		return;
	}
	PKEmitter3D *emitter = cast_to<PKEmitter3D>(p_object);
	if (emitter == nullptr) {
		return;
	}
	add_property_editor("", memnew(PKEmitterPropertiesEditor(emitter)), true);
}

bool PKInspectorPlugin::_can_handle(Object *p_object) const {
	return p_object->is_class(PKEmitter3D::get_class_static());
}
} //namespace godot
