//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_editor_plugin.h"

#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_settings.hpp"

using namespace godot;

void PKEditorPlugin::_enter_tree() {
	_add_editor_setting_ifn(Variant::BOOL, "popcornfx/emitters/foldable_attributes", PROPERTY_HINT_NONE, "", false);

	export_plugin.instantiate();
	add_export_plugin(export_plugin);

	effect_importer.instantiate();
	add_import_plugin(effect_importer);

	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);

	pk_emitter_3d_gizmo_plugin.instantiate();
	add_node_3d_gizmo_plugin(pk_emitter_3d_gizmo_plugin);
}

void PKEditorPlugin::_exit_tree() {
	remove_export_plugin(export_plugin);
	remove_import_plugin(effect_importer);
	remove_inspector_plugin(inspector_plugin);
	remove_node_3d_gizmo_plugin(pk_emitter_3d_gizmo_plugin);
}

void PKEditorPlugin::_add_editor_setting_ifn(Variant::Type p_type, const String &p_name, PropertyHint p_hint, const char *p_hint_string, Variant p_default_value) {
	const Ref<EditorSettings> e_settings = EditorInterface::get_singleton()->get_editor_settings();
	if (!e_settings->has_setting(p_name)) {
		e_settings->set_setting(p_name, p_default_value);
	}
	Dictionary d;
	d["name"] = p_name;
	d["type"] = p_type;
	d["hint"] = p_hint;
	d["hint_string"] = p_hint_string;
	e_settings->add_property_info(d);
	e_settings->set_initial_value(p_name, p_default_value, false);
}
