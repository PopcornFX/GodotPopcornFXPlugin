//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_register_types.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/resource_saver.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include "core/pk_attribute_list.h"
#include "core/pk_audio_effect_capture.h"
#include "core/pk_effect.h"
#include "core/pk_string_names.h"
#include "core/samplers/pk_attribute_sampler_audio.h"
#include "core/samplers/pk_attribute_sampler_image.h"
#include "core/samplers/pk_attribute_sampler_shape.h"
#include "integration/pk_plugin.h"
#include "pk_manager.h"
#include "scene/pk_emitter_3d.h"

#ifdef TOOLS_ENABLED
#include "editor/inspect/pk_emitter_properties_editor.h"
#include "editor/inspect/pk_inspector_plugin.h"
#include "editor/pk_editor_plugin.h"
#endif // TOOLS_ENABLED

#include <gdextension_interface.h>

using namespace godot;

static PKManager *popcornfx_manager = nullptr;
static ObjectID popcornfx_manager_id;
static Ref<ResourceFormatLoaderPKEffect> popcornfx_effect_loader;

void initialize_popcornfx_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		PKPlugin::create_singleton();
		PKStringNames::create_singleton();

		ClassDB::register_class<PKAttributeSampler>();
		ClassDB::register_class<PKAttributeSamplerShape>();
		ClassDB::register_class<PKAttributeSamplerImage>();
		ClassDB::register_class<PKAttributeSamplerAudio>();
		ClassDB::register_class<PKAudioEffectCapture>();
		ClassDB::register_class<PKAudioEffectCaptureInstance>();
		ClassDB::register_class<PKAttributeList>();
		ClassDB::register_class<PKAttributeDesc>();
		ClassDB::register_class<PKAttributeSamplerDesc>();
		ClassDB::register_class<PKEmitter3D>();

		ClassDB::register_class<ResourceFormatLoaderPKEffect>();
		popcornfx_effect_loader.instantiate();
		ResourceLoader::get_singleton()->add_resource_format_loader(popcornfx_effect_loader);

		ClassDB::register_class<PKEffect>();
		ClassDB::register_class<PKManager>();
		popcornfx_manager = memnew(PKManager);
		popcornfx_manager_id = popcornfx_manager->get_instance_id();
		popcornfx_manager->init();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		ClassDB::register_class<PKEffectImporter>();

		if (Engine::get_singleton()->is_editor_hint()) { // Actually in editor, not just embed game.
			ClassDB::register_class<PKExportPlugin>();
			ClassDB::register_class<PKInspectorPlugin>();
			ClassDB::register_class<PKEditorPlugin>();
			popcornfx_manager->add_child(memnew(PKEditorPlugin));

			ClassDB::register_class<PKEmitterPropertiesEditor>();
		}
	}
#endif // TOOLS_ENABLED
}

void uninitialize_popcornfx_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (popcornfx_manager_id.is_valid() && UtilityFunctions::is_instance_id_valid(popcornfx_manager_id)) {
			memdelete(popcornfx_manager);
		}
		popcornfx_manager = nullptr;

		ResourceLoader::get_singleton()->remove_resource_format_loader(popcornfx_effect_loader);
		popcornfx_effect_loader.unref();

		PKStringNames::destroy_singleton();
		PKPlugin::destroy_singleton();
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT popcornfx_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
		const GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_popcornfx_module);
	init_obj.register_terminator(uninitialize_popcornfx_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
