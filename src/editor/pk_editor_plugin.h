//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#ifdef TOOLS_ENABLED

#include "godot_cpp/classes/editor_plugin.hpp"

#include "export/pk_export_plugin.h"
#include "import/pk_effect_importer.h"
#include "inspect/pk_inspector_plugin.h"

namespace godot {

class PKEditorPlugin : public EditorPlugin {
	GDCLASS(PKEditorPlugin, EditorPlugin);

public:
	virtual void _enter_tree() override;
	virtual void _exit_tree() override;

	PKEditorPlugin() = default;

protected:
	static void _bind_methods() {}

	Ref<PKExportPlugin> export_plugin;
	Ref<PKEffectImporter> effect_importer;
	Ref<PKInspectorPlugin> inspector_plugin;
};

} // namespace godot
#endif
