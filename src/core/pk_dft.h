//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once
#include <stdint.h>

class PKDiscreteFourierTransform {
	// For now, we don't function with an instance for the fft, so it's basically a namespace
	// Maybe in the future if the algorithm changes there will be a need for it.
	PKDiscreteFourierTransform() = delete;

public:
	// performs a discrete fourier transform in-place
	static void apply_dft(float *p_buffer, uint32_t p_buffer_size);
};
