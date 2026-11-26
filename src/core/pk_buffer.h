//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/variant/packed_byte_array.hpp"
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
	RID get_srv() const;

private:
	bool _is_init() const { return initialized; }
	void _resize_data();

	bool initialized = false;
	uint32_t mapping_counter = 0;

	// CPU data which gets written to
	PackedByteArray data;

	// Low-level GPU texture to which CPU data gets copied
	RID tex;

	uint32_t size = 0;
};
