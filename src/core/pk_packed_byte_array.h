//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/packed_data_container.hpp"

namespace godot {

class PKPackedByteArray : public PackedByteArray {
public:
	bool resize_aligned(int64_t p_new_size, uint32_t p_align_size_on);
	void clear_slack_data(int64_t p_used_size);

	void encode_vector2(int64_t p_byte_offset, Vector2 p_value);
	void encode_vector3(int64_t p_byte_offset, Vector3 p_value);
	void encode_vector4(int64_t p_byte_offset, Vector4 p_value);
	void encode_color(int64_t p_byte_offset, Color p_value);

	Vector2 decode_vector2(int64_t p_byte_offset) const;
	Vector3 decode_vector3(int64_t p_byte_offset) const;
	Vector4 decode_vector4(int64_t p_byte_offset) const;
	Color decode_color(int64_t p_byte_offset) const;
};

} //namespace godot
