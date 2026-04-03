#ifdef PK_HAS_SOFT_PARTICLES
// Uniforms
uniform float in_softness_distance;
uniform sampler2D in_tex_depth : hint_depth_texture;

// Fragment shader code
void pk_frag_soft_particles(inout vec4 r_albedo, vec2 p_screen_uv, vec3 p_vertex, mat4 p_inv_projection_matrix) {
    float scene_depth_screen = texture(in_tex_depth, p_screen_uv).x;
    vec3 scene_depth_ndc = vec3((p_screen_uv * 2.0) - 1.0, scene_depth_screen);
    vec4 scene_depth_view = p_inv_projection_matrix * vec4(scene_depth_ndc, 1.0);
    scene_depth_view.xyz /= scene_depth_view.w;
    float scene_depth = -scene_depth_view.z;
    float frag_depth = -p_vertex.z;
    float inv_softness_distance = 1.0f / in_softness_distance;
    float depthfade = clamp((scene_depth - frag_depth) * inv_softness_distance, 0.0, 1.0);
    r_albedo *= depthfade;
}
#endif
