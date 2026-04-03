//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include <cmath>

#include "pk_dft.h"

#include "godot_cpp/templates/local_vector.hpp"

using namespace godot;

// Copy of Godot's FFT implementation used in their Spectrum effect.
static void smbFft(float *fftBuffer, long fftFrameSize, long sign)
/*
	FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
	Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the
	time domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes
	and returns the cosine and sine parts in an interleaved manner, ie.
	fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize
	must be a power of 2. It expects a complex input signal (see footnote 2),
	ie. when working with 'common' audio signals our input signal has to be
	passed as {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform
	of the frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(std::log((double)fftFrameSize) / std::log(2.) + .5); k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2 >> 1);
		wr = std::cos(arg);
		wi = sign * std::sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}

// NOTE for now the perfs are good enough. See if this is needed.
#if 0
// Performs two real-valued ffts using the complex-valued fft algorithm, by having z_k = x_k + iy_k.
// Data is already interleaved this way, the important part is how we interpret the result of the dft.
static void two_for_one_fft(float *data, uint32_t halfN) {
	smbFft(data, halfN, -1l); // result if of size N

	// X_k = ( Z_k - conj(Z_(N-k)) ) / 2
	// Y_k = -i ( Z_k - conj(Z_(N-k)) ) / 2 = -iZ_k
	// We don't care about the imaginary part (which represents the phase, we only want amplitudes)
	for (uint32_t k = 0; k < halfN; ++k) {
		float x_k = (data[k] - data[halfN - k]) / 2.0;
		float y_k = (data[k] - data[halfN - k]) / 2.0;
	}
}
#endif

// Does the preprocessing (windowing), fft and post-processing similarily to Godot.
void PKDiscreteFourierTransform::apply_dft(float *p_buffer, uint32_t p_buffer_size) {
	LocalVector<float> fft_buffer;
	fft_buffer.resize(p_buffer_size * 2);
	for (uint32_t i = 0; i < p_buffer_size * 2; i += 2) {
		const float window = -.5 * cos(2. * M_PI * (float)i / (float)p_buffer_size) + .5;
		fft_buffer[i] = p_buffer[i / 2] * window; // real part
		fft_buffer[i + 1] = 0.0f; // imaginary part
	}

	smbFft(fft_buffer.ptr(), p_buffer_size, -1);

	// Output amplitude data is half the size of input for real signals.
	// We could return buffers of half the size or sample twice as big. To discuss, for now we just dup the real part.
	for (uint32_t i = 0; i < p_buffer_size / 2; ++i) {
		p_buffer[2 * i] = sqrtf((fft_buffer[2 * i] * fft_buffer[2 * i]) + (fft_buffer[2 * i + 1] * fft_buffer[2 * i + 1])) / (p_buffer_size / 2.0f);
		p_buffer[2 * i + 1] = p_buffer[2 * i];
	}
}
