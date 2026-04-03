//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_packed_byte_array.h"

#include "integration/pk_plugin.h"

using namespace godot;

bool PKPackedByteArray::resize_aligned(int64_t p_new_size, uint32_t p_align_size_on) {
	const int64_t aligned_size = Mem::Align(p_new_size, p_align_size_on);
	if (aligned_size == size()) {
		return false;
	}
	resize(aligned_size);
	return true;
}

void PKPackedByteArray::clear_slack_data(int64_t p_used_size) {
	p_used_size = MIN(p_used_size, size());
	const int64_t slack = size() - p_used_size;
	if (slack == 0) {
		return;
	}
	memset(ptrw() + p_used_size, 0, slack);
}

void PKPackedByteArray::encode_vector2(int64_t p_byte_offset, Vector2 p_value) {
	const uint32_t elem_size = sizeof(float);
	encode_float(p_byte_offset + elem_size * 0, p_value.x);
	encode_float(p_byte_offset + elem_size * 1, p_value.y);
}

void PKPackedByteArray::encode_vector3(int64_t p_byte_offset, Vector3 p_value) {
	const uint32_t elem_size = sizeof(float);
	encode_float(p_byte_offset + elem_size * 0, p_value.x);
	encode_float(p_byte_offset + elem_size * 1, p_value.y);
	encode_float(p_byte_offset + elem_size * 2, p_value.z);
}

void PKPackedByteArray::encode_vector4(int64_t p_byte_offset, Vector4 p_value) {
	const uint32_t elem_size = sizeof(float);
	encode_float(p_byte_offset + elem_size * 0, p_value.x);
	encode_float(p_byte_offset + elem_size * 1, p_value.y);
	encode_float(p_byte_offset + elem_size * 2, p_value.z);
	encode_float(p_byte_offset + elem_size * 3, p_value.w);
}

void PKPackedByteArray::encode_color(int64_t p_byte_offset, Color p_value) {
	const uint32_t elem_size = sizeof(float);
	encode_float(p_byte_offset + elem_size * 0, p_value.r);
	encode_float(p_byte_offset + elem_size * 1, p_value.g);
	encode_float(p_byte_offset + elem_size * 2, p_value.b);
	encode_float(p_byte_offset + elem_size * 3, p_value.a);
}

Vector2 PKPackedByteArray::decode_vector2(int64_t p_byte_offset) const {
	const uint32_t elem_size = sizeof(float);
	return Vector2(
			decode_float(p_byte_offset + elem_size * 0),
			decode_float(p_byte_offset + elem_size * 1));
}

Vector3 PKPackedByteArray::decode_vector3(int64_t p_byte_offset) const {
	const uint32_t elem_size = sizeof(float);
	return Vector3(
			decode_float(p_byte_offset + elem_size * 0),
			decode_float(p_byte_offset + elem_size * 1),
			decode_float(p_byte_offset + elem_size * 2));
}

Vector4 PKPackedByteArray::decode_vector4(int64_t p_byte_offset) const {
	const uint32_t elem_size = sizeof(float);
	return Vector4(
			decode_float(p_byte_offset + elem_size * 0),
			decode_float(p_byte_offset + elem_size * 1),
			decode_float(p_byte_offset + elem_size * 2),
			decode_float(p_byte_offset + elem_size * 3));
}

Color PKPackedByteArray::decode_color(int64_t p_byte_offset) const {
	const uint32_t elem_size = sizeof(float);
	return Color(
			decode_float(p_byte_offset + elem_size * 0),
			decode_float(p_byte_offset + elem_size * 1),
			decode_float(p_byte_offset + elem_size * 2),
			decode_float(p_byte_offset + elem_size * 3));
}
