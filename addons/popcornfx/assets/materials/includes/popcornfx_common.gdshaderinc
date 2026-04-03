uniform sampler2D in_tex_sim_data;

uint texbuf_width(sampler2D p_texbuf) {
	return uint(textureSize(p_texbuf, 0).x);
}

ivec2 index_to_texcoord(uint p_index, sampler2D p_texbuf) {
	return ivec2(
		int(p_index % texbuf_width(p_texbuf)),
		int(p_index / texbuf_width(p_texbuf))
	);
}

uint texbuf_load_u(uint p_id, uint p_offset, sampler2D p_texbuf) {
	return floatBitsToUint(texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id, p_texbuf), 0).r);
}

float texbuf_load_f(uint p_id, uint p_offset, sampler2D p_texbuf) {
	return texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id, p_texbuf), 0).r;
}

vec2 texbuf_load_2f(uint p_id, uint p_offset, sampler2D p_texbuf) {
	return vec2(
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 2u + 0u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 2u + 1u, p_texbuf), 0).r
	);
}

vec3 texbuf_load_3f(uint p_id, uint p_offset, sampler2D p_texbuf) {
	return vec3(
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 3u + 0u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 3u + 1u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 3u + 2u, p_texbuf), 0).r
	);
}

vec4 texbuf_load_4f(uint p_id, uint p_offset, sampler2D p_texbuf) {
	return vec4(
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 4u + 0u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 4u + 1u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 4u + 2u, p_texbuf), 0).r,
		texelFetch(p_texbuf, index_to_texcoord(p_offset + p_id * 4u + 3u, p_texbuf), 0).r
	);
}
