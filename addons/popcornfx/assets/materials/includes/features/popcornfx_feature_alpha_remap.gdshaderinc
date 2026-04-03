#ifdef PK_HAS_ALPHA_REMAP
// Uniforms
uniform uint in_offset_alpha_cursors;
varying flat float var_alpha_cursor;
uniform sampler2D in_tex_alpha_remap;

// Vertex shader code
float pk_vert_alpha_remap(uint p_particle_id) {
	if (in_offset_alpha_cursors != -1u)
		return texbuf_load_f(p_particle_id, in_offset_alpha_cursors, in_tex_sim_data);
    return 0.0;
}

// Fragment shader code
void pk_frag_alpha_remap(inout float r_alpha) {
    vec2 alpha_coord = vec2(r_alpha, var_alpha_cursor);
    r_alpha = texture(in_tex_alpha_remap, alpha_coord).r;
}
#endif
