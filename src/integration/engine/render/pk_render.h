//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_frame_collector.h"
#include "pk_frame_collector_types.h"

class PKRenderDataFactory;

class PKRender {
public:
	enum RenderMode {
		RENDER_MODE_3D,
		RENDER_MODE_2D,
	};

	PKRender(RenderMode p_render_mode);
	~PKRender();

	void init(CParticleMediumCollection *p_medium_collection);
	void render_3d(TArray<SSceneView> &p_scene_views, RID p_scenario_id);
	void render_2d(TArray<SSceneView> &p_scene_views, RID p_canvas_id);

private:
	RenderMode render_mode;

	PKRenderDataFactory *render_batch_factory;
	PKFrameCollector frame_collector;
	PKRenderContext render_context;

	CParticleMediumCollection *medium_collection = nullptr;
};
