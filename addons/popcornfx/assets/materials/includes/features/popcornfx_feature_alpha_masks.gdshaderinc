#ifdef PK_HAS_ALPHA_MASKS
// Uniforms
uniform uint in_offset_alpha_mask_1_animation_cursor;
uniform uint in_offset_alpha_mask_2_animation_cursor;

uniform float in_alpha_mask_1_intensity;
uniform float in_alpha_mask_1_weight;
uniform vec2 in_alpha_mask_1_scale;
uniform float in_alpha_mask_1_rotation_speed;
uniform vec2 in_alpha_mask_1_translation_speed;

uniform float in_alpha_mask_2_intensity;
uniform float in_alpha_mask_2_weight;
uniform vec2 in_alpha_mask_2_scale;
uniform float in_alpha_mask_2_rotation_speed;
uniform vec2 in_alpha_mask_2_translation_speed;

uniform sampler2D in_tex_alpha_mask_1_map : repeat_enable;
uniform sampler2D in_tex_alpha_mask_2_map : repeat_enable;

varying float var_alpha_mask_1_animation_cursor;
varying float var_alpha_mask_2_animation_cursor;

vec2 transform_uv(vec2 uv, vec2 scale, mat2 rotation, vec2 offset)
{
    return rotation * (uv * scale) + offset;
}

// Vertex shader code
float pk_vert_alpha_masks_animation_cursor_1(uint p_particle_id) {
	if (in_offset_alpha_mask_1_animation_cursor != -1u)
        return texbuf_load_f(p_particle_id, in_offset_alpha_mask_1_animation_cursor, in_tex_sim_data);
    return 0.0;
}

float pk_vert_alpha_masks_animation_cursor_2(uint p_particle_id) {
	if (in_offset_alpha_mask_2_animation_cursor != -1u)
        return texbuf_load_f(p_particle_id, in_offset_alpha_mask_2_animation_cursor, in_tex_sim_data);
    return 0.0;
}

// Fragment shader code
void pk_frag_alpha_masks(inout FragSamplingParams r_params) {
    vec2 uv_mask_1 = r_params.raw_uv;
    vec2 uv_mask_2 = r_params.raw_uv;

    float angle_1 = -in_alpha_mask_1_rotation_speed * var_alpha_mask_1_animation_cursor;
    float sin_1 = sin(angle_1);
    float cos_1 = cos(angle_1);
    mat2 uv_rotation_1 = mat2(vec2(cos_1, sin_1), vec2(-sin_1, cos_1));

    vec2 translation_1 = in_alpha_mask_1_translation_speed * var_alpha_mask_1_animation_cursor;
    vec2 scale_1 = in_alpha_mask_1_scale;

    uv_mask_1 = transform_uv(uv_mask_1 - 0.5f, scale_1, uv_rotation_1, translation_1 + scale_1 * 0.5f);

    float mask_sample_1 = texture(in_tex_alpha_mask_1_map, uv_mask_1).r * in_alpha_mask_1_intensity;
    r_params.alpha_mask_sample1 = mix(mask_sample_1, 1.0f, 1.0f - in_alpha_mask_1_weight);

    float angle_2 = -in_alpha_mask_2_rotation_speed * var_alpha_mask_2_animation_cursor;
    float sin_2 = sin(angle_2);
    float cos_2 = cos(angle_2);
    mat2 uv_rotation_2 = mat2(vec2(cos_2, sin_2), vec2(-sin_2, cos_2));

    vec2 translation_2 = in_alpha_mask_2_translation_speed * var_alpha_mask_2_animation_cursor;
    vec2 scale_2 = in_alpha_mask_2_scale;

    uv_mask_2 = transform_uv(uv_mask_2 - 0.5f, scale_2, uv_rotation_2, translation_2 + scale_2 * 0.5f);

    float mask_sample_2 = texture(in_tex_alpha_mask_2_map, uv_mask_2).r * in_alpha_mask_2_intensity;
    r_params.alpha_mask_sample2 = mix(mask_sample_2, 1.0f, 1.0f - in_alpha_mask_2_weight);
}

#ifdef PK_HAS_EMISSIVE
void pk_frag_apply_final_alpha_masks(inout vec4 r_albedo, inout vec3 r_emission, FragSamplingParams p_params)
#else
void pk_frag_apply_final_alpha_masks(inout vec4 r_albedo, FragSamplingParams p_params)
#endif
{
    float alpha_product = 1.0f;

    alpha_product *= p_params.alpha_mask_sample1 * p_params.alpha_mask_sample2;

#ifdef PK_HAS_EMISSIVE
    r_emission *= clamp(alpha_product, 0.0f, 1.0f);
#endif

#ifdef PK_HAS_DIFFUSE
    r_albedo *= clamp(alpha_product, 0.0f, 1.0f);
#endif
}
#endif
