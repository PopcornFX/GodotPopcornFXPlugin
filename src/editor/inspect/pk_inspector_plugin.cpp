//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_inspector_plugin.h"

#include "godot_cpp/classes/editor_inspector.hpp"

#include "pk_editor_property_bool_array.h"
#include "scene/pk_emitter_3d.h"

namespace godot {
bool PKInspectorPlugin::_can_handle(Object *p_object) const {
	return p_object->is_class(PKEmitter3D::get_class_static());
}

bool PKInspectorPlugin::_parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
	switch (p_type) {
		case Variant::ARRAY: {
			if (p_hint_type == PROPERTY_HINT_NONE && p_hint_string.begins_with("PKAttributeBool")) {
				const String rest = p_hint_string.trim_prefix("PKAttributeBool");
				if (!rest.is_valid_int()) {
					return false;
				}
				PKEditorPropertyBoolArray *property = memnew(PKEditorPropertyBoolArray(rest.to_int()));
				property->set_label(p_name);
				add_property_editor(p_name, property, false);
				return true;
			}
			return false;
		}
	}

	return false;
}

} //namespace godot
