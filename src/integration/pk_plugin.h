//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_sdk.h"

#include "internal/pk_project_settings.h"
#include "internal/pk_scene.h"
#include "pk_error_handling.h"
#include "pk_plugin_types.h"

#ifdef TOOLS_ENABLED
class PKBaker;
#endif //TOOLS_ENABLED
class PKRender;
class PKScene;

namespace godot {

struct Projection;
struct Transform3D;

class PKPlugin {
	static PKPlugin *singleton;

public:
	PKPlugin();
	~PKPlugin();

	static PKPlugin *get_singleton();
	static void create_singleton();
	static void destroy_singleton();

	// This must be called before startup
	void set_project_settings(const SPopcornFXProjectSettings &p_settings);
	const PKProjectSettings *get_project_settings() const;
#ifdef TOOLS_ENABLED
	PKBaker *get_baker() const;
#endif //TOOLS_ENABLED
	PKScene *get_scene_3d() const;
	PKScene *get_scene_2d() const;

	bool startup();
	void shutdown();

	void update_3d(float p_delta, const Transform3D &p_cam_view, const Projection &p_cam_proj) const;
	void update_2d(float p_delta, const Transform2D &p_cam_view);
	void render_3d(RID p_scenario_id) const;
	void render_2d(RID p_canvas_id) const;

private:
	bool launched_popcornfx;

#ifdef TOOLS_ENABLED
	PKBaker *baker;
#endif //TOOLS_ENABLED
	SPopcornFXProjectSettings init_project_settings;
	PKProjectSettings *project_settings;

	PKScene *scene_3d;
	PKScene *scene_2d;
	PKRender *renderer_3d;
	PKRender *renderer_2d;
};

} //namespace godot
