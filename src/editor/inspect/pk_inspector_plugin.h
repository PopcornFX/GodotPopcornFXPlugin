//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/editor_inspector_plugin.hpp"

#include "integration/pk_plugin.h"

namespace godot {
class PKInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(PKInspectorPlugin, EditorInspectorPlugin);

public:
	virtual void _parse_category(Object *p_object, const String &p_category) override;
	virtual bool _can_handle(Object *p_object) const override;

protected:
	static void _bind_methods() {}
};
} //namespace godot
