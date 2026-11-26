//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_buffer.h"

#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/classes/rendering_server.hpp"

#include "integration/pk_error_handling.h"

using RS = RenderingServer;

void PKBuffer::init() {
	size = 0;
	initialized = true;
}

void PKBuffer::destroy() {
	PKGD_ASSERT_MSG(mapping_counter == 0, "Destroying buffer while it is still mapped!");

	if (tex.is_valid()) {
		RS::get_singleton()->free_rid(tex);
		tex = RID();
	}
	size = 0;
	initialized = false;
}

void PKBuffer::resize(uint32_t p_size_in_bytes) {
	if (!PKGD_VERIFY_MSG(_is_init() && mapping_counter == 0, "Can't resize if buffer isn't init or is mapped!")) {
		return;
	}

	// Compute necessary texture dimensions (power of two square texture of float1)
	const uint32_t pixel_count = p_size_in_bytes / sizeof(float);
	const uint32_t tex_dimensions = IntegerTools::NextOrEqualPowerOfTwo((uint32_t)PKFloatToIntCeil(sqrtf((float)pixel_count)));

	if (tex_dimensions == size || tex_dimensions == 0) {
		return;
	}
	size = tex_dimensions;

	RS *rs = RS::get_singleton();
	const Ref<Image> image = Image::create_empty(size, size, false, Image::FORMAT_RF);
	const RID new_tex = rs->texture_2d_create(image);
	if (tex.is_valid()) {
		rs->texture_replace(tex, new_tex);
	} else {
		tex = new_tex;
	}
}

uint8_t *PKBuffer::map() {
	PKGD_ASSERT_MSG(!is_empty() && mapping_counter == 0, "Shouldn't map if buffer is empty or already mapped!");
	++mapping_counter;

	_resize_data();
	return data.ptrw();
}

void PKBuffer::unmap() {
	if (!PKGD_VERIFY_MSG(mapping_counter > 0 && --mapping_counter == 0, "Can't unmap if not mapped before or mapped more than once!")) {
		return;
	}

	_resize_data();
	const Ref<Image> image = Image::create_from_data(size, size, false, Image::FORMAT_RF, data);
	RS::get_singleton()->texture_2d_update(tex, image, 0);
}

uint32_t PKBuffer::capacity_in_bytes() const {
	return size * size * sizeof(float);
}

RID PKBuffer::get_srv() const {
	if (!PKGD_VERIFY_MSG(is_valid(), "Shouldn't use buffer SRV if it hasn't been init and resized!")) {
		return RID();
	}
	return tex;
}

void PKBuffer::_resize_data() {
	const uint32_t byte_size = capacity_in_bytes();
	if (data.size() != byte_size) {
		data.resize(byte_size);
	}
}
