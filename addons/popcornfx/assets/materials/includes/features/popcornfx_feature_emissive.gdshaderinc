#ifdef PK_HAS_EMISSIVE
// Uniforms
uniform uint in_offset_emissive_colors3;
uniform uint in_offset_emissive_colors4;
varying flat vec4 var_emissive_color;
uniform sampler2D in_tex_emissive : source_color;
#ifdef PK_HAS_EMISSIVE_RAMP
	uniform sampler2D in_tex_emissive_ramp;
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
    #ifndef PK_HAS_TRANSFORM_UVS
		vec4 tex_val_emissive = texture(in_tex_emissive, p_params.uv);
	#else
		vec4 tex_val_emissive = textureGrad(in_tex_emissive, p_params.uv, p_params.dUVdx, p_params.dUVdy);
	#endif

	#ifdef PK_HAS_ATLAS_BLEND
		#ifndef PK_HAS_TRANSFORM_UVS
			vec4 tex_val_emissive2 = texture(in_tex_emissive, p_params.uv2);
		#else
			vec4 tex_val_emissive2 = textureGrad(in_tex_emissive, p_params.uv2, p_params.dUVdx, p_params.dUVdy);
		#endif
		tex_val_emissive = mix(tex_val_emissive, tex_val_emissive2, p_params.blend_mix);
	#endif

	#ifdef PK_HAS_EMISSIVE_RAMP
		tex_val_emissive.rgb = texture(in_tex_emissive_ramp, vec2(tex_val_emissive.x, 0.0)).rgb;
	#endif

	r_emission = var_emissive_color.rgb * tex_val_emissive.rgb
				* var_emissive_color.a * tex_val_emissive.a;
}
#endif