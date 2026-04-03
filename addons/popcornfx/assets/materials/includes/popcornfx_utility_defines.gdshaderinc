#if defined(PK_HAS_ATLAS_BLEND_SOFT) || defined(PK_HAS_ATLAS_BLEND_MV)
	#define PK_HAS_ATLAS_BLEND
#endif
#if defined(PK_HAS_LIGHTING) || defined(PK_HAS_LIGHTING_LEGACY)
	#define PK_HAS_ANY_LIGHTING
#endif

struct FragSamplingParams {
	vec2 uv;
	#ifdef PK_HAS_ATLAS_BLEND
		vec2 uv2;
		float blend_mix;
	#endif
	#ifdef PK_HAS_TRANSFORM_UVS
		bool transform_uv_rgb_only;
		vec2 dUVdx;
		vec2 dUVdy;
		vec2 old_uv;
		#ifdef PK_HAS_ATLAS
			vec4 rect;
			#ifdef PK_HAS_ATLAS_BLEND
				vec2 old_uv2;
				vec4 rect2;
			#endif
		#endif
	#endif
};

FragSamplingParams build_frag_sampling_params(vec2 p_uv, vec2 p_uv2) {
	FragSamplingParams params;

	params.uv = p_uv;
	#ifdef PK_HAS_ATLAS_BLEND
		params.uv2 = p_uv2;
		params.blend_mix = 0.0;
	#endif
	#ifdef PK_HAS_TRANSFORM_UVS
		params.transform_uv_rgb_only = false;
		params.dUVdx = vec2(0.0);
		params.dUVdy = vec2(0.0);
		params.old_uv = p_uv;
		#ifdef PK_HAS_ATLAS
			params.rect = vec4(1.0, 1.0, 0.0, 0.0);
			#ifdef PK_HAS_ATLAS_BLEND
				params.old_uv2 = p_uv2;
				params.rect2 = vec4(1.0, 1.0, 0.0, 0.0);
			#endif
		#endif
	#endif

	return params;
}
