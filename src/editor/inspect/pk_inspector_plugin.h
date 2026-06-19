//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/editor_inspector_plugin.hpp"

namespace godot {
class PKInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(PKInspectorPlugin, EditorInspectorPlugin);

public:
	virtual bool _can_handle(Object *p_object) const override;
	virtual bool _parse_property(Object *p_object, Variant::Type p_type, const String &p_name, PropertyHint p_hint_type, const String &p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;

protected:
	static void _bind_methods() {}
};
} //namespace godot
