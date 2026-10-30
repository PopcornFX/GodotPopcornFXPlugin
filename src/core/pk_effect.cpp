//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_effect.h"

#include "godot_cpp/classes/config_file.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

#include "integration/engine/pk_file_system_controller.h"
#include "integration/internal/pk_project_settings.h"
#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"
#include "pk_manager.h"

#include <pk_kernel/include/kr_file.h>
#include <pk_kernel/include/kr_file_controller.h>
#include <pk_particles/include/ps_effect.h>
#include <pk_particles/include/ps_event_map.h>
#include <pk_particles/include/ps_resources.h>

using namespace godot;

Error PKEffect::load(const String &p_path) {
	load_path = p_path;

	IFileSystem *file_system = File::DefaultFileSystem();
	uint32_t raw_file_size = 0; // will be filled by 'Bufferize' call below
	const CString path = to_pk(p_path);
	uint8_t *raw_file_buffer = file_system->Bufferize(path, &raw_file_size, true);
	if (raw_file_buffer == nullptr) {
		return FAILED;
	}

	const String config_path = p_path + String(".cfg");
	if (FileAccess::file_exists(config_path)) {
		Ref<ConfigFile> config;
		config.instantiate();
		config->load(config_path);
		z_index = config->get_value("import_options", "z_index", 0);
		PKManager::get_singleton()->set_effect_z_index(PKFileSystemController::sanitize_physical_path(p_path), z_index);
	}

	CConstMemoryStream memory_file_view(raw_file_buffer, raw_file_size);
	effect = CParticleEffect::Load(memory_file_view, path, SEffectLoadCtl::kDefault, HBO::g_Context, "Default");
	PK_FREE(raw_file_buffer);
	if (!PKGD_VERIFY_MSG(effect != nullptr, "Failed to load the PopcornFX effect %s", path.Data())) {
		return FAILED;
	}

	const PCEventConnectionMap event_connection_map = effect->EventConnectionMap();
	if (!PKGD_VERIFY_MSG(effect != nullptr, "Failed to load the PopcornFX effect's connection map: %s", path.Data())) {
		return FAILED;
	}

	// TODO: would it be possible to regain an early install & the shader prewarm? (had to remove this for 2D support's multi scenes)
	// PKScene *scene = PKPlugin::get_singleton()->get_scene();
	// for (const CEventConnectionMap::SLayerDefinition &layer_def : event_connection_map->m_LayerSlots) {
	// 	PParticleDescriptor descriptor = layer_def.m_ParentDescriptor;
	// 	for (const PRendererDataBase &renderer : descriptor->Renderers()) {
	// 		scene->get_or_create_shader_material(renderer.Get());
	// 	}
	// }

	if (!PKGD_VERIFY_MSG(effect->LoadConnectionMap(), "Failed to load PopcornFX effect's connection map: %s", p_path.utf8().ptr())) {
		return FAILED;
	}

	return OK;
}

PParticleEffectInstance PKEffect::create_instance(PKScene *p_scene) {
	if (effect == nullptr) {
		return nullptr;
	}

	if (p_scene == nullptr) {
		CLog::Log(PK_ERROR, "PKEffect failed to instantiate effect: PopcornFX Scene is null.");
		return nullptr;
	}
	if (!PKGD_VERIFY(effect->Install(p_scene->get_particle_medium_collection()))) {
		return nullptr;
	}
	PParticleEffectInstance instance = effect->Instantiate(p_scene->get_particle_medium_collection());
	if (instance == nullptr) {
		CLog::Log(PK_ERROR, "PKEffect failed to instantiate effect: %s.", load_path.utf8().ptr());
		return nullptr;
	}
	return instance;
}

PKEffect::~PKEffect() {
	effect = nullptr;
}

void PKEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load", "load_path"), &PKEffect::load);
	ClassDB::bind_method(D_METHOD("_get_load_path"), &PKEffect::get_load_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_path", PROPERTY_HINT_FILE, "*.pkb"), "load", "_get_load_path");
	BIND_BASIC_PROPERTY(PKEffect, INT, z_index, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY);
}

Variant ResourceFormatLoaderPKEffect::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {
	Ref<PKEffect> r_effect;
	r_effect.instantiate();

	const Error err = r_effect->load(p_path);
	if (err != OK) {
		return err;
	}

	return r_effect;
}

PackedStringArray ResourceFormatLoaderPKEffect::_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.append("pkb");
	return extensions;
}

bool ResourceFormatLoaderPKEffect::_handles_type(const StringName &p_type) const {
	return p_type == String("PKEffect");
}

String ResourceFormatLoaderPKEffect::_get_resource_type(const String &p_path) const {
	const String ext = p_path.get_extension().to_lower();
	if (ext == "pkb") {
		return "PKEffect";
	}
	return "";
}

PackedStringArray ResourceFormatLoaderPKEffect::_get_dependencies(const String &p_path, bool p_add_types) const {
	PackedStringArray dependencies;
	dependencies.append("res://addons/popcornfx/popcornfx.gdextension"); // No other way to export an extension properly
	const CString &source_pack = PKPlugin::get_singleton()->get_project_settings()->source_pack_root_dir();
	Ref<PKEffect> r_effect;
	r_effect.instantiate();

	const Error err = r_effect->load(p_path);
	ERR_FAIL_COND_V_MSG(err != OK, dependencies, vformat("Could not load the PopcornFX effect to get its dependencies: %s", p_path));

	dependencies.append(p_path + String(".cfg"));

	TArray<SResourceDependency> pk_dependencies;
	r_effect->get_effect()->GatherRuntimeDependencies(pk_dependencies);
	for (size_t i = 0; i < pk_dependencies.Count(); i++) {
		if (pk_dependencies[i].m_Path.EndsWith("pkma")) {
			continue;
		}
		dependencies.append(to_gd(source_pack / pk_dependencies[i].m_Path));
	}

	return dependencies;
}
