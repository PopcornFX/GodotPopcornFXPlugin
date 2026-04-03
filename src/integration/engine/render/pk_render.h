//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_frame_collector.h"
#include "pk_frame_collector_types.h"
#include "pk_render_data_factory.h"

class PKRender {
public:
	PKRender() = default;
	~PKRender();

	void init(CParticleMediumCollection *p_medium_collection);
	void destroy();
	void render(TArray<SSceneView> &p_scene_views, RID p_scenario_id);

private:
	PKRenderDataFactory render_batch_factory;
	PKFrameCollector frame_collector;
	PKRenderContext render_context;

	CParticleMediumCollection *medium_collection = nullptr;
};
