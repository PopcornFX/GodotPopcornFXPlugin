//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_export_plugin.h"

#ifdef TOOLS_ENABLED
#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

namespace godot {

void PKExportPlugin::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	raw_export = ProjectSettings::get_singleton()->get_setting("popcornfx/export/raw_export_dependencies", true);
}

void PKExportPlugin::_export_file(const String &p_path, const String &p_type, const PackedStringArray &p_features) {
	if (p_path.ends_with(".pkma")) {
		skip();
	} else if (p_path.ends_with(".pkat")) {
		if (!raw_export) {
			_copy_file(p_path); // If we're raw-exporting all dependencies, this will get copied below
		}
		skip();
	} else if (p_path.ends_with(".pkfx") && raw_export) {
		ResourceLoader *loader = ResourceLoader::get_singleton();
		PackedStringArray deps = loader->get_dependencies(p_path);
		for (const String &dep : deps) {
			_copy_file(dep);
		}
	}
}

void PKExportPlugin::_copy_from_recursive(const String &p_path) {
	const PackedStringArray files = DirAccess::get_files_at(p_path);
	for (const String &file : files) {
		_copy_file(p_path.path_join(file));
	}

	const PackedStringArray dirs = DirAccess::get_directories_at(p_path);
	for (const String &dir : dirs) {
		_copy_from_recursive(p_path.path_join(dir));
	}
}

void PKExportPlugin::_copy_file(const String &p_path) {
	const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
	if (file.is_valid()) {
		add_file(p_path, file->get_buffer(file->get_length()), false);
	}
}
} //namespace godot

#endif // TOOLS_ENABLED
