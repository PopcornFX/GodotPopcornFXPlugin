//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/shader_material.hpp"
#include "godot_cpp/classes/texture2d.hpp"

#include "integration/pk_sdk.h"

#include <pk_particles/include/Renderers/ps_renderer_enums.h>

namespace PopcornFX {
PK_FORWARD_DECLARE(RendererDataBase);
} //namespace PopcornFX
using namespace godot;

class PKRendererCache;

class PKShaderMaterial {
public:
	enum class BlendMode : uint8_t {
		OPAQUE = 0,
		MASKED,
		ADDITIVE,
		ADDITIVE_NO_ALPHA,
		ALPHA_BLEND,
		PREMUL_ALPHA,
	};
	enum class VariationFlags : uint32_t {
		NONE = 0,
		LIGHTING = (1 << 0),
		LIGHTING_LEGACY = (1 << 1),
		DOUBLE_SIDED = (1 << 2),
		DIFFUSE = (1 << 3),
		DIFFUSE_RAMP = (1 << 4),
		EMISSIVE = (1 << 5),
		EMISSIVE_RAMP = (1 << 6),
		ALPHA_REMAP = (1 << 7),
		ALPHA_MASKS = (1 << 8),
		SIZE_2D = (1 << 9),
		ATLAS = (1 << 10),
		ATLAS_BLEND_SOFT = (1 << 11),
		ATLAS_BLEND_MV = (1 << 12),
		TRANSFORM_UVS = (1 << 13),
		SOFT_PARTICLES = (1 << 14),
		CORRECT_DEFORMATION = (1 << 15),
		DISTORTION = (1 << 16),
		UV_DISTORTIONS = (1 << 17),
		DISSOLVE = (1 << 18),
		VAT_FLUID = (1 << 19),
		VAT_SOFT = (1 << 20),
		VAT_RIGID = (1 << 21),
		SKELETAL_ANIM = (1 << 22),
		SKELETAL_INTERPOL = (1 << 23),
		SKELETAL_TRACK_INTERPOL = (1 << 24),
		BILLBOARD_CAPSULE = (1 << 25),
		MAX = 26
	};

	// Returns the name of the given shader blend mode
	static String get_blend_mode_name(BlendMode p_mode);

	// Returns the name of the given shader variation
	// Note: Only 1 flag must be set, otherwise returns empty string
	static String get_variation_flag_name(VariationFlags p_flag);

	Ref<ShaderMaterial> material;
	ERendererClass renderer_type = (ERendererClass)0;
	BlendMode blend_mode = (BlendMode)0;
	VariationFlags variation_flags = VariationFlags::NONE;

	int32_t transparent_sort_override = 0;
	float transparent_sort_offset = 0;
	float mask_threshold = 0;

	float roughness = 1;
	float metalness = 0;

	float atlas_sub_div_x = 1;
	float atlas_sub_div_y = 1;

	float mv_distortion_strength_columns = 1;
	float mv_distortion_strength_rows = 1;

	float softness_distance = 0;
	bool transform_uvs_rgb_only = false;
	bool cast_shadows = false;

	Ref<Texture2D> tex_diffuse;
	Ref<Texture2D> tex_diffuse_ramp;
	Ref<Texture2D> tex_emissive;
	Ref<Texture2D> tex_emissive_ramp;
	Ref<Texture2D> tex_normal;
	Ref<Texture2D> tex_rough_metal;
	Ref<Texture2D> tex_specular;
	Ref<Texture2D> tex_alpha_remap;
	Ref<Texture2D> tex_motion_vectors;

	Ref<Mesh> mesh;

	void init_from_renderer(const CRendererDataBase &p_renderer);
	void create_material_instance();
	void apply_to_visual_instance(RID p_instance) const;
	void bind_material_properties(const PKRendererCache *p_renderer_cache) const;

	bool operator==(const PKShaderMaterial &p_other) const;

	bool is_transparent() const;
	bool needs_depth_sorting() const;
	bool has_lighting() const;
	bool has_atlas_blending() const;
	bool needs_atlas_rects() const;
	bool has_variation_flag(VariationFlags p_flag) const;
	void set_variation_flag(VariationFlags p_flag);

private:
	String _get_shader_path() const;
};
