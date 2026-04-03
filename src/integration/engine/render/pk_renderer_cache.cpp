//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_renderer_cache.h"

#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>

PKRendererCache::~PKRendererCache() {
	atlas_rects.destroy();
}

void PKRendererCache::UpdateThread_BuildBillboardingFlags(const PRendererDataBase &p_renderer) {
	// Here we define which geometry we need to render this material.
	// m_Flags is used to tell PopcornFX what it needs to generate

	PKGD_ASSERT(p_renderer != null);

	PKScene *scene = PKPlugin::get_singleton()->get_scene();
	const PKShaderMaterial &shader_mat = scene->get_or_create_shader_material(p_renderer.Get());

	m_Flags.m_HasUV = true;
	m_Flags.m_FlipU = false;
	m_Flags.m_FlipV = false;
	m_Flags.m_RotateTexture = false;
	m_Flags.m_HasRibbonCorrectDeformation = false;
	m_Flags.m_HasAtlasBlending = shader_mat.has_atlas_blending();
	m_Flags.m_NeedSort = shader_mat.needs_depth_sorting();
	m_Flags.m_HasNormal = shader_mat.has_lighting();
	m_Flags.m_HasTangent = m_Flags.m_HasNormal;
	m_Flags.m_Slicable = false; // TODO

	if (p_renderer->m_RendererType == PopcornFX::Renderer_Billboard) {
		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_FlipUVs())) { // Legacy
			m_Flags.m_FlipU = false;
			m_Flags.m_FlipV = true;
		}

		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipU())) {
			m_Flags.m_FlipU = true;
		}

		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipV())) {
			m_Flags.m_FlipV = true;
		}

		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_RotateUV())) {
			m_Flags.m_RotateTexture = true;
		}
	}

	else if (p_renderer->m_RendererType == PopcornFX::Renderer_Ribbon) {
		if (p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_TextureUVs_FlipU(), false) ||
				p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipU(), false)) {
			m_Flags.m_FlipU = true;
		}

		if (p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_TextureUVs_FlipV(), false) ||
				p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipV(), false)) {
			m_Flags.m_FlipV = true;
		}

		if (p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_TextureUVs_RotateTexture(), false) ||
				p_renderer->m_Declaration.GetPropertyValue_B(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_RotateUV(), false)) {
			m_Flags.m_RotateTexture = true;
		}

		m_Flags.m_HasRibbonCorrectDeformation = p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_CorrectDeformation());
	}

	else if (p_renderer->m_RendererType == PopcornFX::Renderer_Triangle) {
		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipU())) {
			m_Flags.m_FlipU = true;
		}

		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_FlipV())) {
			m_Flags.m_FlipV = true;
		}

		if (p_renderer->m_Declaration.IsFeatureEnabled(PopcornFX::BasicRendererProperties::SID_BasicTransformUVs_RotateUV())) {
			m_Flags.m_RotateTexture = true;
		}
	}

	else if (p_renderer->m_RendererType == PopcornFX::Renderer_Mesh) {
		if (shader_mat.mesh.is_valid()) {
			m_PerLODMeshCount.Resize(1);
			m_PerLODMeshCount[0] = 1;
		}
	}
}

bool PKRendererCache::UpdateThread_LoadRendererAtlas(const PRendererDataBase &p_renderer, CResourceManager *p_resource_manager) {
	if (!CRendererCacheBase::UpdateThread_LoadRendererAtlas(p_renderer, p_resource_manager)) {
		return false;
	}
	if (m_Atlas == nullptr) {
		return false;
	}

	const TMemoryView<const CFloat4> rects = m_Atlas->m_RectsFp32.View();
	const uint32_t rect_count = m_Atlas->AtlasRectCount();
	atlas_rects.init();
	atlas_rects.resize(sizeof(uint32_t) + rects.CoveredBytes());
	uint8_t *mapped_rects = atlas_rects.map();
	memcpy(mapped_rects, &rect_count, sizeof(uint32_t));
	memcpy(mapped_rects + sizeof(uint32_t), rects.Data(), rects.CoveredBytes());
	atlas_rects.unmap();
	return true;
}

Ref<Texture2DRD> PKRendererCache::get_atlas_rects_srv() const {
	return atlas_rects.get_srv();
}

bool PKRendererCache::operator==(const PKRendererCache &p_other) const {
	return true;
}
