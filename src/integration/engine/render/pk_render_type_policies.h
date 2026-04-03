//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_error_handling.h"

struct PKAdditionalInput {
	uint32_t buffer_offset = 0;
	uint32_t byte_size = 0;
	uint32_t additional_input_index = 0;
};

struct PKStreamOffset {
	enum {
		INVALID = -1u
	};

	uint32_t offset_in_bytes = INVALID;

	void reset() { offset_in_bytes = INVALID; }
	bool is_valid() const { return offset_in_bytes != INVALID; }
	uint32_t offset_for_shader_constant() const { return is_valid() ? offset_in_bytes / sizeof(float) : INVALID; }

	operator uint32_t() const {
		PKGD_VERIFY(is_valid());
		return offset_in_bytes;
	}
	PKStreamOffset &operator=(uint32_t p_offset_in_bytes) {
		offset_in_bytes = p_offset_in_bytes;
		return *this;
	}
};
