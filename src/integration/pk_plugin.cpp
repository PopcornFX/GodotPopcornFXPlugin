//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_plugin.h"

#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"

#include "engine/render/pk_render.h"
#include "internal/pk_project_settings.h"
#include "internal/pk_startup.h"

#ifdef TOOLS_ENABLED
#include "internal/editor/pk_baker.h"
#endif //TOOLS_ENABLED

namespace godot {

PKPlugin *PKPlugin::singleton;

PKPlugin::PKPlugin() {
	launched_popcornfx = false;
	scene_3d = nullptr;
	scene_2d = nullptr;
#ifdef TOOLS_ENABLED
	baker = nullptr;
#endif //TOOLS_ENABLED
}

PKPlugin::~PKPlugin() {
	// Shutdown popcorn here because shutdown is called too early, PK objects get deleted after
	if (launched_popcornfx) {
		popcornfx_shutdown();
		launched_popcornfx = false;
	}
}

PKPlugin *PKPlugin::get_singleton() {
	PKGD_ASSERT(singleton != nullptr);
	return singleton;
}

void PKPlugin::create_singleton() {
	PKGD_ASSERT(singleton == nullptr);
	singleton = memnew(PKPlugin);
}

void PKPlugin::destroy_singleton() {
	PKGD_ASSERT(singleton != nullptr);
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
	engine_editor_settings.unit_pixel_size = p_settings.unit_pixel_size;
	project_settings->set_settings(engine_editor_settings);
	project_settings->load_ifn();
}

const PKProjectSettings *PKPlugin::get_project_settings() const {
	return project_settings;
}

#ifdef TOOLS_ENABLED
PKBaker *PKPlugin::get_baker() const {
	return baker;
}
#endif //TOOLS_ENABLED

PKScene *PKPlugin::get_scene_3d() const {
	return scene_3d;
}

PKScene *PKPlugin::get_scene_2d() const {
	return scene_2d;
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

	scene_3d = PK_NEW(PKScene(PKRender::RENDER_MODE_3D));
	ERR_FAIL_COND_V_MSG(scene_3d == nullptr, false, "Couldn't create PopcornFX 3D scene.");
	scene_2d = PK_NEW(PKScene(PKRender::RENDER_MODE_2D));
	ERR_FAIL_COND_V_MSG(scene_2d == nullptr, false, "Couldn't create PopcornFX 2D scene.");
	renderer_3d = PK_NEW(PKRender(PKRender::RENDER_MODE_3D));
	ERR_FAIL_COND_V_MSG(renderer_3d == nullptr, false, "Couldn't create PopcornFX Godot 3D renderer.");
	renderer_2d = PK_NEW(PKRender(PKRender::RENDER_MODE_2D));
	ERR_FAIL_COND_V_MSG(renderer_2d == nullptr, false, "Couldn't create PopcornFX Godot 2D renderer.");

	renderer_3d->init(scene_3d->get_particle_medium_collection());
	renderer_2d->init(scene_2d->get_particle_medium_collection());

	return true;
}

void PKPlugin::shutdown() {
#ifdef TOOLS_ENABLED
	PK_SAFE_DELETE(baker);
#endif //TOOLS_ENABLED

	PK_SAFE_DELETE(project_settings);
	PK_SAFE_DELETE(renderer_3d);
	PK_SAFE_DELETE(renderer_2d);
	PK_SAFE_DELETE(scene_3d);
	PK_SAFE_DELETE(scene_2d);
}

void PKPlugin::update_3d(float p_delta, const Transform3D &p_cam_view, const Projection &p_cam_proj) const {
	PKGD_ASSERT(launched_popcornfx == true);
	PKGD_ASSERT(scene_3d != nullptr);

	CFloat4x4 mat_wv = to_pk(p_cam_view);
	CFloat4x4 mat_wvp = mat_wv * to_pk(p_cam_proj);
	scene_3d->update(p_delta, mat_wv, mat_wvp);
}

void PKPlugin::update_2d(float p_delta, const Transform2D &p_cam_view) {
	PKGD_ASSERT(launched_popcornfx == true);
	PKGD_ASSERT(scene_2d != nullptr);
	Transform2D cam_view = p_cam_view;
	cam_view.columns[2] /= get_project_settings()->unit_pixel_size();
	CFloat4x4 mat_wv = to_pk(cam_view);
	mat_wv.Translations().z() = 1'000'000; // Turn ViewAligned billboards to be flush with screen.
	scene_2d->update(p_delta, mat_wv, mat_wv);
}

void PKPlugin::render_3d(RID p_scenario_id) const {
	PKGD_ASSERT(launched_popcornfx == true);
	PKGD_ASSERT(scene_3d != nullptr);

	renderer_3d->render_3d(scene_3d->get_views(), p_scenario_id);
}

void PKPlugin::render_2d(RID p_canvas_id) const {
	PKGD_ASSERT(launched_popcornfx == true);
	PKGD_ASSERT(scene_2d != nullptr);

	renderer_2d->render_2d(scene_2d->get_views(), p_canvas_id);
}

} // namespace godot
