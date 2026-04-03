#ifdef PK_HAS_ANY_LIGHTING
// Uniforms
uniform sampler2D in_tex_normal : hint_normal;
#ifdef PK_HAS_LIGHTING
	uniform float in_roughness;
	uniform float in_metalness;
	uniform sampler2D in_tex_rough_metal;
#endif
#ifdef PK_HAS_LIGHTING_LEGACY
	uniform sampler2D in_tex_specular;
#endif

// Fragment shader code
void pk_frag_lighting(out vec3 r_normal_map, out vec3 r_rough_metal_specular, FragSamplingParams p_params)
{
	#ifndef PK_HAS_TRANSFORM_UVS
		vec3 tex_val_normal = texture(in_tex_normal, p_params.uv).xyz;
	#else
		vec3 tex_val_normal = textureGrad(in_tex_normal, p_params.uv, p_params.dUVdx, p_params.dUVdy).xyz;
	#endif

	#ifdef PK_HAS_ATLAS_BLEND
		#ifndef PK_HAS_TRANSFORM_UVS
			vec3 tex_val_normal2 = texture(in_tex_normal, p_params.uv2).xyz;
		#else
			vec3 tex_val_normal2 = textureGrad(in_tex_normal, p_params.uv2, p_params.dUVdx, p_params.dUVdy).xyz;
		#endif
		tex_val_normal = mix(tex_val_normal, tex_val_normal2, p_params.blend_mix);
	#endif

	r_normal_map = tex_val_normal;

#if defined(PK_HAS_LIGHTING)
	#ifndef PK_HAS_TRANSFORM_UVS
		vec2 tex_val_rough_metal = texture(in_tex_rough_metal, p_params.uv).xy;
	#else
		vec2 tex_val_rough_metal = textureGrad(in_tex_rough_metal, p_params.uv, p_params.dUVdx, p_params.dUVdy).xy;
	#endif

	#ifdef PK_HAS_ATLAS_BLEND
		#ifndef PK_HAS_TRANSFORM_UVS
			vec2 tex_val_rough_metal2 = texture(in_tex_rough_metal, p_params.uv2).xy;
		#else
			vec2 tex_val_rough_metal2 = textureGrad(in_tex_rough_metal, p_params.uv2, p_params.dUVdx, p_params.dUVdy).xy;
		#endif

		tex_val_rough_metal = mix(tex_val_rough_metal, tex_val_rough_metal2, p_params.blend_mix);
	#endif

	r_rough_metal_specular.x = in_roughness * tex_val_rough_metal.x;
	r_rough_metal_specular.y = in_metalness * tex_val_rough_metal.y;

#elif defined(PK_HAS_LIGHTING_LEGACY)
	#ifndef PK_HAS_TRANSFORM_UVS
		float tex_val_specular = texture(in_tex_specular, p_params.uv).x;
	#else
		float tex_val_specular = textureGrad(in_tex_specular, p_params.uv, p_params.dUVdx, p_params.dUVdy).x;
	#endif

	#ifdef PK_HAS_ATLAS_BLEND
		#ifndef PK_HAS_TRANSFORM_UVS
			float tex_val_specular2 = texture(in_tex_specular, p_params.uv2).x;
		#else
			float tex_val_specular2 = texture(in_tex_specular, p_params.uv2, p_params.dUVdx, p_params.dUVdy).x;
		#endif
		tex_val_specular = mix(tex_val_specular, tex_val_specular2, p_params.blend_mix);
	#endif

	r_rough_metal_specular.z = tex_val_specular;
#endif
}
#endif
