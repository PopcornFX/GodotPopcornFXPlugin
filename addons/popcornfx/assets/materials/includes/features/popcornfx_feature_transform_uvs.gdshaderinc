#ifdef PK_HAS_TRANSFORM_UVS
// Uniforms
uniform uint in_offset_uv_offsets;
uniform uint in_offset_uv_rotations;
uniform uint in_offset_uv_scales;
varying flat vec2 var_uv_offset;
varying flat float var_uv_rotation;
varying flat vec2 var_uv_scale;
uniform bool in_transform_uv_rgb_only;

// Vertex shader code
vec2 pk_vert_transform_uvs_offset(uint p_particle_id) {
    if (in_offset_uv_offsets != -1u)
		return texbuf_load_2f(p_particle_id, in_offset_uv_offsets, in_tex_sim_data);
	return vec2(0.0);
}
float pk_vert_transform_uvs_rotation(uint p_particle_id) {
	if (in_offset_uv_rotations != -1u)
		return texbuf_load_f(p_particle_id, in_offset_uv_rotations, in_tex_sim_data);
	return 0.0;
}
vec2 pk_vert_transform_uvs_scale(uint p_particle_id) {
	if (in_offset_uv_scales != -1u)
		return texbuf_load_2f(p_particle_id, in_offset_uv_scales, in_tex_sim_data);
	return vec2(0.0);
}

// Fragment shader code
void pk_frag_transform_uvs(inout FragSamplingParams r_params)
{
    float sin_r = sin(var_uv_rotation);
	float cos_r = cos(var_uv_rotation);
	mat2 uv_rotation = mat2(vec2(cos_r, sin_r), vec2(-sin_r, cos_r));
	vec2 uv_scale = var_uv_scale;
	vec2 uv_offset = var_uv_offset;

#ifdef PK_HAS_ATLAS
	vec4 rect = r_params.rect;
#else
	vec4 rect = vec4(1.0, 1.0, 0.0, 0.0);
#endif
	
	r_params.old_uv = r_params.uv;
	r_params.uv = ((r_params.uv - rect.zw) / rect.xy); // normalize (if atlas)
	r_params.uv = uv_rotation * (r_params.uv * uv_scale) + uv_offset; // scale then rotate then translate UV
	// For clean derivatives:
	vec2 _uv = r_params.uv * rect.xy + rect.zw;
	r_params.dUVdx = dFdx(_uv);
	r_params.dUVdy = dFdy(_uv);
	r_params.uv = fract(r_params.uv) * rect.xy + rect.zw; // undo normalize
	
#ifdef PK_HAS_ATLAS_BLEND
	vec4 rect2 = r_params.rect2;
	r_params.old_uv2 = r_params.uv2;
	r_params.uv2 = ((r_params.uv2 - rect2.zw) / rect2.xy); // normalize (if atlas)
	r_params.uv2 = uv_rotation * (r_params.uv2 * uv_scale) + uv_offset; // scale then rotate then translate UV
	r_params.uv2 = fract(r_params.uv2) * rect2.xy + rect2.zw; // undo normalize
#endif

	r_params.transform_uv_rgb_only = in_transform_uv_rgb_only;
}
#endif
