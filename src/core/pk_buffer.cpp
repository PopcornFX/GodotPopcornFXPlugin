//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_buffer.h"

#include "godot_cpp/classes/rendering_server.hpp"

#include "integration/pk_error_handling.h"

using RS = RenderingServer;
using RD = RenderingDevice;

void PKBuffer::init() {
	srv.instantiate();
	view.instantiate();
	format.instantiate();
	format->set_format(RD::DATA_FORMAT_R32_SFLOAT);
	format->set_usage_bits(RD::TEXTURE_USAGE_CAN_UPDATE_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT);
	format->set_width(0);
	format->set_height(0);
	format->set_depth(1);
	format->set_mipmaps(1);
}

void PKBuffer::destroy() {
	PKGD_ASSERT_MSG(mapping_counter == 0, "Destroying buffer while it is still mapped!");

	if (srv.is_valid()) {
		srv->set_texture_rd_rid(RID());
	}

	if (tex.is_valid()) {
		RD *rd = RS::get_singleton()->get_rendering_device();
		rd->free_rid(tex);
	}
}

void PKBuffer::resize(uint32_t p_size_in_bytes) {
	if (!PKGD_VERIFY_MSG(_is_init() && mapping_counter == 0, "Can't resize if buffer isn't init or is mapped!")) {
		return;
	}

	// Compute necessary texture dimensions (power of two square texture of float1)
	const uint32_t pixel_count = p_size_in_bytes / sizeof(float);
	const uint32_t tex_dimensions = IntegerTools::NextOrEqualPowerOfTwo((uint32_t)PKFloatToIntCeil(sqrtf((float)pixel_count)));

	// Update texture format dimensions and reallocate the texture
	if (tex_dimensions != format->get_width()) {
		format->set_width(tex_dimensions);
		format->set_height(tex_dimensions);

		RD *rd = RS::get_singleton()->get_rendering_device();

		// Allocate a new low-level texture and link it to the high-level wrapper object
		const RID new_tex = rd->texture_create(format, view);
		srv->set_texture_rd_rid(new_tex);

		// Free the old texture RID now that it isn't referenced in the wrapper anymore
		if (tex.is_valid()) {
			rd->free_rid(tex);
		}
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
	RD *rd = RS::get_singleton()->get_rendering_device();
	rd->texture_update(srv->get_texture_rd_rid(), 0, data);
}

uint32_t PKBuffer::capacity_in_bytes() const {
	const uint32_t tex_dimensions = format->get_width();
	return tex_dimensions * tex_dimensions * sizeof(float);
}

Ref<Texture2DRD> PKBuffer::get_srv() const {
	if (!PKGD_VERIFY_MSG(is_valid(), "Shouldn't use buffer SRV if it hasn't been init and resized!")) {
		return nullptr;
	}
	return srv;
}

void PKBuffer::_resize_data() {
	const uint32_t byte_size = capacity_in_bytes();
	if (data.size() != byte_size) {
		data.resize(byte_size);
	}
}
