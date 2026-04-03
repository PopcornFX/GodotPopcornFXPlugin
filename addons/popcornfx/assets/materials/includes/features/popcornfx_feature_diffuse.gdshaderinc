#ifdef PK_HAS_DIFFUSE
// Uniforms
uniform uint in_offset_colors;
varying flat vec4 var_diffuse_color;
uniform sampler2D in_tex_diffuse : source_color;
#ifdef PK_HAS_DIFFUSE_RAMP
    uniform sampler2D in_tex_diffuse_ramp;
#endif

// Vertex shader code
vec4 pk_vert_diffuse(uint p_particle_id) {
    if (in_offset_colors != -1u)
        return texbuf_load_4f(p_particle_id, in_offset_colors, in_tex_sim_data);
    return vec4(0.0);
}

// Fragment shader code
void pk_frag_diffuse(out vec4 r_albedo, FragSamplingParams p_params)
{
    #ifndef PK_HAS_TRANSFORM_UVS
        vec4 tex_val_diffuse = texture(in_tex_diffuse, p_params.uv);
    #else
        vec4 tex_val_diffuse = textureGrad(in_tex_diffuse, p_params.uv, p_params.dUVdx, p_params.dUVdy);
        if (p_params.transform_uv_rgb_only) {
            tex_val_diffuse.a = texture(in_tex_diffuse, p_params.old_uv).a;
        }
    #endif

    #ifdef PK_HAS_ATLAS_BLEND
        #ifndef PK_HAS_TRANSFORM_UVS
            vec4 tex_val_diffuse2 = texture(in_tex_diffuse, p_params.uv2);
        #else
            vec4 tex_val_diffuse2 = textureGrad(in_tex_diffuse, p_params.uv2, p_params.dUVdx, p_params.dUVdy);
            if (p_params.transform_uv_rgb_only) {
                tex_val_diffuse2.a = texture(in_tex_diffuse, p_params.old_uv2).a;
            }
        #endif
        tex_val_diffuse = mix(tex_val_diffuse, tex_val_diffuse2, p_params.blend_mix);
    #endif

    #ifdef PK_HAS_DIFFUSE_RAMP
        tex_val_diffuse.rgb = texture(in_tex_diffuse_ramp, vec2(tex_val_diffuse.x, 0.0)).rgb;
    #endif

    r_albedo = var_diffuse_color * tex_val_diffuse;
}
#endif
