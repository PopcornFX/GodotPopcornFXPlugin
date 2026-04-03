//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_editor_plugin.h"

using namespace godot;

void PKEditorPlugin::_enter_tree() {
	export_plugin.instantiate();
	add_export_plugin(export_plugin);

	effect_importer.instantiate();
	add_import_plugin(effect_importer);

	inspector_plugin.instantiate();
	add_inspector_plugin(inspector_plugin);
}

void PKEditorPlugin::_exit_tree() {
	remove_export_plugin(export_plugin);
	remove_import_plugin(effect_importer);
	remove_inspector_plugin(inspector_plugin);
}
