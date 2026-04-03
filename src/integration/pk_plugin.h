//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_sdk.h"

#include "pk_error_handling.h"
#include "pk_plugin_types.h"

#ifdef TOOLS_ENABLED
class PKBaker;
#endif //TOOLS_ENABLED
class PKProjectSettings;

class PKScene;
class PKRender;

namespace godot {

class Projection;
class Transform3D;

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
	const PKProjectSettings *get_processed_project_settings();
#ifdef TOOLS_ENABLED
	PKBaker *get_baker();
#endif //TOOLS_ENABLED
	PKScene *get_scene();

	bool startup();
	void shutdown();

	void update(float p_delta, const Transform3D &p_cam_view, const Projection &p_cam_proj);
	void render(RID p_scenario_id);

private:
	bool launched_popcornfx;

#ifdef TOOLS_ENABLED
	PKBaker *baker;
#endif //TOOLS_ENABLED
	SPopcornFXProjectSettings init_project_settings;
	PKProjectSettings *project_settings;

	PKScene *scene;
	PKRender *renderer;
};

} //namespace godot
