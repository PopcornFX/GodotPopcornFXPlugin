shader_type spatial;
render_mode cull_disabled;

#include "includes/popcornfx_utility_defines.gdshaderinc"

#if defined(PK_BLEND_PREMUL_ALPHA)
render_mode blend_premul_alpha;
#elif defined(PK_BLEND_ADDITIVE) || defined(PK_BLEND_ADDITIVE_NO_ALPHA)
render_mode blend_add;
#else
render_mode blend_mix;
#endif

#ifndef PK_HAS_ANY_LIGHTING
render_mode unshaded;
#endif

#include "includes/popcornfx_common.gdshaderinc"
#include "includes/features/popcornfx_feature_alpha_remap.gdshaderinc"
#include "includes/features/popcornfx_feature_atlas_billboard.gdshaderinc"
#include "includes/features/popcornfx_feature_diffuse.gdshaderinc"
#include "includes/features/popcornfx_feature_emissive.gdshaderinc"
#include "includes/features/popcornfx_feature_lighting.gdshaderinc"
#include "includes/features/popcornfx_feature_soft_particles.gdshaderinc"
#include "includes/features/popcornfx_feature_transform_uvs.gdshaderinc"

#ifdef PK_BLEND_MASKED
	uniform float in_mask_threshold;
#endif

void vertex() {
	uint particle_id = uint(VERTEX_ID / 4);
	
#ifdef PK_HAS_DIFFUSE
	var_diffuse_color = pk_vert_diffuse(particle_id);
#endif
#ifdef PK_HAS_EMISSIVE
	var_emissive_color = pk_vert_emissive(particle_id);
#endif
#ifdef PK_HAS_ALPHA_REMAP
	var_alpha_cursor = pk_vert_alpha_remap(particle_id);
#endif
#ifdef PK_HAS_ATLAS
	var_texture_id = pk_vert_atlas(particle_id);
#endif
#ifdef PK_HAS_TRANSFORM_UVS
	var_uv_offset = pk_vert_transform_uvs_offset(particle_id);
	var_uv_rotation = pk_vert_transform_uvs_rotation(particle_id);
	var_uv_scale = pk_vert_transform_uvs_scale(particle_id);
#endif
}

void fragment() {
	FragSamplingParams params = build_frag_sampling_params(UV, UV2);

	// Compute each rendering feature
#ifdef PK_HAS_ATLAS
	pk_frag_atlas(params);
#endif
#ifdef PK_HAS_TRANSFORM_UVS
	pk_frag_transform_uvs(params);
#endif
	vec4 albedo = vec4(0.0);
#ifdef PK_HAS_DIFFUSE
	pk_frag_diffuse(albedo, params);
#endif
#ifdef PK_HAS_ALPHA_REMAP
	pk_frag_alpha_remap(albedo.a);
#endif
#ifdef PK_HAS_SOFT_PARTICLES
	pk_frag_soft_particles(albedo, SCREEN_UV, VERTEX, INV_PROJECTION_MATRIX);
#endif
#ifdef PK_HAS_EMISSIVE
	vec3 emission = vec3(0.0);
	pk_frag_emissive(emission, params);
#endif
#ifdef PK_HAS_ANY_LIGHTING
	vec3 normal_map = vec3(0.0);
	vec3 rough_metal_specular = vec3(0.0);
	pk_frag_lighting(normal_map, rough_metal_specular, params);
#endif

	// Apply computed albedo and alpha values
	ALBEDO = albedo.rgb;
#if defined(PK_BLEND_ADDITIVE) || defined(PK_BLEND_ALPHA) || defined(PK_BLEND_PREMUL_ALPHA)
	ALPHA = albedo.a;
#elif defined(PK_BLEND_MASKED)
	if (albedo.a < in_mask_threshold) {
		discard;
	}
#endif
#if defined(PK_BLEND_PREMUL_ALPHA) && defined(PK_HAS_ANY_LIGHTING)
	// According to docs, this should only be written to when using a shaded material
	PREMUL_ALPHA_FACTOR = albedo.a;
#endif

	// Apply computed emissive values
#ifdef PK_HAS_EMISSIVE
	#ifdef PK_HAS_ANY_LIGHTING
		EMISSION = emission;
	#else
		// 'render_mode unshaded' disables EMISSION, so add emissive to ALBEDO
		ALBEDO += emission;
	#endif
#endif

	// Apply computed lighting values
#ifdef PK_HAS_ANY_LIGHTING
	NORMAL_MAP = normal_map;
#endif
#ifdef PK_HAS_LIGHTING
	ROUGHNESS = rough_metal_specular.x;
	METALLIC = rough_metal_specular.y;
#endif
#ifdef PK_HAS_LIGHTING_LEGACY
	SPECULAR = rough_metal_specular.z;
#endif
}
