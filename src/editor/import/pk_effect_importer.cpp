//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#if TOOLS_ENABLED
#include "pk_effect_importer.h"

#include "godot_cpp/classes/dir_access.hpp"

#include "core/pk_effect.h"
#include "integration/internal/editor/pk_baker.h"
#include "integration/pk_plugin.h"

using namespace godot;

String PKEffectImporter::_get_importer_name() const {
	return "PKEffectImporter";
}

String PKEffectImporter::_get_visible_name() const {
	return "PopcornFX Effect";
}

int32_t PKEffectImporter::_get_preset_count() const {
	return 0;
}

String PKEffectImporter::_get_preset_name(int32_t p_preset_index) const {
	return "";
}

PackedStringArray PKEffectImporter::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.append("pkfx");
	return extensions;
}

TypedArray<Dictionary> PKEffectImporter::_get_import_options(const String &p_path, int32_t p_preset_index) const {
	TypedArray<Dictionary> output;
	return output;
}

String PKEffectImporter::_get_save_extension() const {
	return "pkb";
}

String PKEffectImporter::_get_resource_type() const {
	return "PKEffect";
}

float PKEffectImporter::_get_priority() const {
	return 1.0;
}

int32_t PKEffectImporter::_get_import_order() const {
	return IMPORT_ORDER_DEFAULT;
}

bool PKEffectImporter::_get_option_visibility(const String &p_path, const StringName &p_option_name, const Dictionary &p_options) const {
	return true;
}

Error PKEffectImporter::_import(const String &p_source_file, const String &p_save_path, const Dictionary &p_options, const TypedArray<String> &r_platform_variants, const TypedArray<String> &r_gen_files) const {
	const String effect_save_path = p_save_path + String(".") + _get_save_extension();

	PKBaker *baker = PKPlugin::get_singleton()->get_baker();

	if (baker != nullptr) {
		if (!baker->bake_effect(to_pk(p_source_file), to_pk(effect_save_path))) {
			return FAILED;
		}
	}
	const int idx = effect_save_path.find(".pkfx");
	if (idx >= 0) {
		const Ref<DirAccess> dir = DirAccess::open("res://");
		const String new_path = effect_save_path.erase(idx, effect_save_path.length() - idx) + ".pkfx";
		dir->rename(new_path, effect_save_path);
		CParticleEffect::Unload(to_pk(effect_save_path)); // Unload effect so that next load doesn't use cached version
	}
	return OK;
}
#endif
