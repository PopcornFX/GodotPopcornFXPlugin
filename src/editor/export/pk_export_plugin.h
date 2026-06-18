//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#ifdef TOOLS_ENABLED

#include "godot_cpp/classes/editor_export_platform.hpp"
#include "godot_cpp/classes/editor_export_plugin.hpp"
#include "godot_cpp/templates/hash_set.hpp"

namespace godot {

class PKExportPlugin : public EditorExportPlugin {
	GDCLASS(PKExportPlugin, EditorExportPlugin);

public:
	virtual String _get_name() const override { return "PopcornFX"; }
	virtual void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
	virtual void _export_file(const String &p_path, const String &p_type, const PackedStringArray &p_features) override;

protected:
	static void _bind_methods() {}

private:
	void _copy_from_recursive(const String &p_path);
	void _copy_file(const String &p_path);

	bool raw_export;
};

} // namespace godot
#endif
