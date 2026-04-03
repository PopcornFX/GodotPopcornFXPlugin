#ifdef PK_HAS_ATLAS
// Uniforms
uniform uint in_offset_texture_ids;
varying flat float var_texture_id;
#ifdef PK_HAS_ATLAS_BLEND_MV
	uniform vec2 in_atlas_mv_strength;
	uniform sampler2D in_tex_atlas_mv : hint_normal;
#endif
#ifdef PK_HAS_TRANSFORM_UVS
    uniform sampler2D in_tex_atlas_rects;
#endif

// Vertex shader code
float pk_vert_atlas(uint p_particle_id) {
#if defined(PK_HAS_ATLAS_BLEND) || defined(PK_HAS_TRANSFORM_UVS)
    // Texture ID only needs to be fetched if blending or transform UVs is enabled
    if (in_offset_texture_ids != -1u)
		return texbuf_load_f(p_particle_id, in_offset_texture_ids, in_tex_sim_data);
#endif
    return 0.0;
}

// Fragment shader code
void pk_frag_atlas(inout FragSamplingParams r_params) {
#ifdef PK_HAS_ATLAS_BLEND
    r_params.blend_mix = fract(var_texture_id);
    
    #ifdef PK_HAS_ATLAS_BLEND_MV
        // TODO: Same calculation as SampleLib, but different result with default texture
        // (possibly because the mv texture is sRGB...)
        vec2 tex_val_mv = ((texture(in_tex_atlas_mv, r_params.uv).rg * 2.0) - 1.0);
        vec2 tex_val_mv2 = ((texture(in_tex_atlas_mv, r_params.uv2).rg * 2.0) - 1.0);
        vec2 cur_vectors = tex_val_mv * in_atlas_mv_strength * r_params.blend_mix;
        vec2 next_vectors = tex_val_mv2 * in_atlas_mv_strength * (1.0 - r_params.blend_mix);
        r_params.uv -= cur_vectors;
        r_params.uv2 += next_vectors;
    #endif
#endif

#ifdef PK_HAS_TRANSFORM_UVS
    // Atlas rects only need to be fetched and stored if transform UVs is active
    uint rect_count = texbuf_load_u(0, 0, in_tex_atlas_rects);

    uint rect_id = min(rect_count - 1u, uint(var_texture_id));
    r_params.rect = texbuf_load_4f(rect_id, 1, in_tex_atlas_rects);

    #ifdef PK_HAS_ATLAS_BLEND
        uint rect2_id = min(rect_count - 1u, uint(var_texture_id) + 1u);
        r_params.rect2 = texbuf_load_4f(rect2_id, 1, in_tex_atlas_rects);
    #endif
#endif
}
#endif
