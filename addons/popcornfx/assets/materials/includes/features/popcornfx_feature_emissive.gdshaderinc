#ifdef PK_HAS_EMISSIVE
// Uniforms
uniform uint in_offset_emissive_colors3;
uniform uint in_offset_emissive_colors4;
#if defined(PK_RENDERER_RIBBON) || defined(PK_RENDERER_MESH)
varying vec4 var_emissive_color;
#else
varying flat vec4 var_emissive_color;
#endif
uniform sampler2D in_tex_emissive : source_color;
#ifdef PK_HAS_EMISSIVE_RAMP
	uniform sampler2D in_tex_emissive_ramp : source_color, repeat_disable, filter_nearest;
#endif

// Vertex shader code
vec4 pk_vert_emissive(uint p_particle_id) {
    if (in_offset_emissive_colors4 != -1u)
		return texbuf_load_4f(p_particle_id, in_offset_emissive_colors4, in_tex_sim_data);
	else if (in_offset_emissive_colors3 != -1u)
		return vec4(texbuf_load_3f(p_particle_id, in_offset_emissive_colors3, in_tex_sim_data), 1);
	return vec4(0.0);
}

// Fragment shader code
void pk_frag_emissive(out vec3 r_emission, FragSamplingParams p_params)
{
    #if !defined(PK_HAS_TRANSFORM_UVS)
		vec4 tex_val_emissive = texture(in_tex_emissive, p_params.uv);
	#else
		vec4 tex_val_emissive = textureGrad(in_tex_emissive, p_params.uv, p_params.dUVdx, p_params.dUVdy);
	#endif

	#ifdef PK_HAS_ATLAS_BLEND
		#if !defined(PK_HAS_TRANSFORM_UVS)
			vec4 tex_val_emissive2 = texture(in_tex_emissive, p_params.uv2);
		#else
			vec4 tex_val_emissive2 = textureGrad(in_tex_emissive, p_params.uv2, p_params.dUVdx, p_params.dUVdy);
		#endif
		tex_val_emissive = mix(tex_val_emissive, tex_val_emissive2, p_params.blend_mix);
	#endif

	r_emission = var_emissive_color.rgb * tex_val_emissive.rgb * var_emissive_color.a * tex_val_emissive.a;

	#ifdef PK_HAS_EMISSIVE_RAMP
		r_emission.rgb = texture(in_tex_emissive_ramp, vec2(r_emission.r, 0.0)).rgb;
	#endif
}
#endif