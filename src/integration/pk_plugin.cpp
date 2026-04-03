//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_plugin.h"

#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"

#include "engine/render/pk_render.h"
#include "internal/pk_project_settings.h"
#include "internal/pk_scene.h"
#include "internal/pk_startup.h"

#ifdef TOOLS_ENABLED
#include "internal/editor/pk_baker.h"
#endif //TOOLS_ENABLED

namespace godot {

PKPlugin *PKPlugin::singleton;

PKPlugin::PKPlugin() {
	launched_popcornfx = false;
	scene = nullptr;
#ifdef TOOLS_ENABLED
	baker = nullptr;
#endif //TOOLS_ENABLED
}

PKPlugin::~PKPlugin() {
	// Shutdown popcorn here because shutdown is called too early, PK objects get deleted after
	if (launched_popcornfx) {
		popcornfx_shutdown();
	}
}

PKPlugin *PKPlugin::get_singleton() {
	DEV_ASSERT(singleton != nullptr);
	return singleton;
}

void PKPlugin::create_singleton() {
	DEV_ASSERT(singleton == nullptr);
	singleton = memnew(PKPlugin);
}

void PKPlugin::destroy_singleton() {
	DEV_ASSERT(singleton != nullptr);
	memdelete(singleton);
	singleton = nullptr;
}

void PKPlugin::set_project_settings(const SPopcornFXProjectSettings &p_settings) {
	PKProjectSettings::EngineSettings engine_editor_settings;
#if TOOLS_ENABLED
	engine_editor_settings.source_pack = to_pk(p_settings.source_pack);
	engine_editor_settings.debug_baked_effects = p_settings.debug_baked_effects;
#endif
	engine_editor_settings.source_pack_root = to_pk(p_settings.source_pack_root);
	project_settings->set_settings(engine_editor_settings);
	project_settings->load_ifn();
}

const PKProjectSettings *PKPlugin::get_processed_project_settings() {
	return project_settings;
}

#ifdef TOOLS_ENABLED
PKBaker *PKPlugin::get_baker() {
	return baker;
}
#endif //TOOLS_ENABLED

PKScene *PKPlugin::get_scene() {
	return scene;
}

bool PKPlugin::startup() {
	launched_popcornfx = popcornfx_startup();
	ERR_FAIL_COND_V_MSG(!launched_popcornfx, false, "Couldn't startup PopcornFX.");

#ifdef TOOLS_ENABLED
	baker = PK_NEW(PKBaker);
	ERR_FAIL_COND_V_MSG(baker == nullptr, false, "Couldn't create PopcornFX baker.");
#endif //TOOLS_ENABLED

	project_settings = PK_NEW(PKProjectSettings);
	ERR_FAIL_COND_V_MSG(project_settings == nullptr, false, "Couldn't create PopcornFX project settings.");

	scene = PK_NEW(PKScene);
	ERR_FAIL_COND_V_MSG(scene == nullptr, false, "Couldn't create PopcornFX scene.");
	renderer = PK_NEW(PKRender);
	ERR_FAIL_COND_V_MSG(renderer == nullptr, false, "Couldn't create PopcornFX Godot renderer.");

	renderer->init(scene->get_particle_medium_collection());

	return true;
}

void PKPlugin::shutdown() {
#ifdef TOOLS_ENABLED
	PK_SAFE_DELETE(project_settings);
	PK_SAFE_DELETE(baker);
#endif //TOOLS_ENABLED

	PK_SAFE_DELETE(renderer);
	PK_SAFE_DELETE(scene);
}

void PKPlugin::update(float p_delta, const Transform3D &p_cam_view, const Projection &p_cam_proj) {
	DEV_ASSERT(launched_popcornfx == true);
	DEV_ASSERT(scene != nullptr);

	CFloat4x4 mat_wv = to_pk(p_cam_view);
	CFloat4x4 mat_wvp = mat_wv * to_pk(p_cam_proj);
	scene->update(p_delta, mat_wv, mat_wvp);
}

void PKPlugin::render(RID p_scenario_id) {
	DEV_ASSERT(launched_popcornfx == true);
	DEV_ASSERT(scene != nullptr);

	renderer->render(scene->get_views(), p_scenario_id);
}

} // namespace godot
