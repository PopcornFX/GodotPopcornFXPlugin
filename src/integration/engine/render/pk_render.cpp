//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_render.h"

PKRender::~PKRender() {
	destroy();
}

void PKRender::init(CParticleMediumCollection *p_medium_collection) {
	const uint32_t enabled_renderers = (1U << ERendererClass::Renderer_Billboard) |
			(1U << ERendererClass::Renderer_Ribbon) |
			(1U << ERendererClass::Renderer_Mesh); /*|
			(1U << ERendererClass::Renderer_Light) |
			(1U << ERendererClass::Renderer_Decal) |
			(1U << ERendererClass::Renderer_Sound)*/
	;

	CFrameCollector::SFrameCollectorInit init(enabled_renderers,
			CbNewBatchDrawer(&render_batch_factory, &PKRenderDataFactory::create_batch_drawer),
			CbNewRendererCache(&render_batch_factory, &PKRenderDataFactory::create_renderer_cache),
			2,
			false);

	medium_collection = p_medium_collection;

	frame_collector.Initialize(init);
	frame_collector.InstallToMediumCollection(medium_collection);
}

void PKRender::destroy() {
	if (medium_collection != nullptr) {
		frame_collector.UninstallFromMediumCollection(medium_collection);
		medium_collection = nullptr;
	}

	frame_collector.ReleaseRenderedFrame();
	frame_collector.Destroy();
}

void PKRender::render(TArray<SSceneView> &p_scene_views, RID p_scenario_id) {
	frame_collector.CollectFrame();

	SParticleCollectedFrameToRender2 *new_to_render = frame_collector.GetLastCollectedFrame();
	if (new_to_render != nullptr) {
		frame_collector.BuildNewFrame(new_to_render);

		const uint32_t slices_max_count = 1;
		frame_collector.SetDrawCallsSortMethod(slices_max_count == 1 ? Sort_None : Sort_Slices);

		render_context.m_Views = p_scene_views;
		render_context.scenario_id = p_scenario_id;

		if (frame_collector.BeginRenderBuiltFrame(render_context)) {
			frame_collector.EndRenderBuiltFrame(render_context, true);
		}
	}
}
