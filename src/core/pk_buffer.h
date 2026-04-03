//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/rid.hpp"

using namespace godot;

// GPU texture that acts like a readonly buffer of floats
// In shaders:
// - This buffer has the type: sampler2D
// - 1D buffer indices must be converted to 2D
//       x = idx % textureSize().x
//       y = idx / textureSize().x
// - Use texelFetch().r to access the value at a 2D index
class PKBuffer {
public:
	// Initializes internal objects, must be called before anything else
	void init();

	// Frees internal objects, must be called before going out of scope
	void destroy();

	// Updates the GPU texture's capacity to contain at least the requested number of bytes
	void resize(uint32_t p_size_in_bytes);

	// Returns a byte pointer to CPU data, which gets resized if necessary
	uint8_t *map();

	// Copies CPU data to the GPU texture
	void unmap();

	bool is_valid() const { return _is_init() && tex.is_valid(); }
	bool is_empty() const { return !is_valid() || capacity_in_bytes() == 0; }
	uint32_t capacity_in_bytes() const;
	Ref<Texture2DRD> get_srv() const;

private:
	bool _is_init() const { return format.is_valid() && view.is_valid() && srv.is_valid(); }
	void _resize_data();

	uint32_t mapping_counter = 0;

	// CPU data which gets written to
	PackedByteArray data;

	// Low-level GPU texture to which CPU data gets copied
	RID tex;

	// Format of the GPU texture (hardcoded to float1 pixel format)
	Ref<RDTextureFormat> format;

	// High-level texture object used to wrap our texture RID (to be used as a ShaderMaterial parameter)
	Ref<Texture2DRD> srv;

	// Image view for the texture (only used by RenderingDevice internally)
	Ref<RDTextureView> view;
};
