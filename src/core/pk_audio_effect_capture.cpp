//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

// This is a modified version of Godot's AudioEffectCapture.

#include "godot_cpp/classes/audio_server.hpp"

#include "pk_audio_effect_capture.h"
#include "pk_manager.h"

using namespace godot;

void PKAudioEffectCaptureInstance::_process(const void *p_src_frames, AudioFrame *r_frames, int32_t p_frame_count) {
	PopcornFXRingBuffer<AudioFrame> &buffer = base->buffer;
	const AudioFrame *src_frames = reinterpret_cast<const AudioFrame *>(p_src_frames);
	for (int i = 0; i < p_frame_count; i++) {
		r_frames[i] = src_frames[i];
	}

	// Add incoming audio frames to the IO ring buffer
	int32_t ret = buffer.write_override(src_frames, p_frame_count);
	ERR_FAIL_COND_MSG(ret != p_frame_count, "Failed to add data to effect capture ring buffer.");
}

bool PKAudioEffectCaptureInstance::_process_silence() const {
	return true;
}

PKAudioEffectCapture::PKAudioEffectCapture() {
	set_buffer_length(PKManager::AUDIO_BUFFER_SIZE);
}

Ref<AudioEffectInstance> PKAudioEffectCapture::_instantiate() {
	if (!buffer_initialized) {
		ERR_FAIL_COND_V(buffer_length_samples <= 0 || buffer_length_samples >= (1 << 27), Ref<AudioEffectInstance>());
		buffer.resize(nearest_shift(buffer_length_samples));
		buffer_initialized = true;
	}

	clear_buffer();

	Ref<PKAudioEffectCaptureInstance> ins;
	ins.instantiate();
	ins->base = Ref<PKAudioEffectCapture>(this);

	return ins;
}

void PKAudioEffectCapture::set_buffer_length(uint32_t p_buffer_length_samples) {
	ERR_FAIL_COND(p_buffer_length_samples <= 0 || p_buffer_length_samples >= (1 << 27));
	buffer_length_samples = p_buffer_length_samples;
	buffer.resize(nearest_shift(buffer_length_samples));
}

uint32_t PKAudioEffectCapture::get_buffer_length() {
	return buffer_length_samples;
}

uint32_t PKAudioEffectCapture::get_frames_left() const {
	ERR_FAIL_COND_V(!buffer_initialized, 0);
	return buffer.data_left();
}

bool PKAudioEffectCapture::can_get_buffer(int p_frames) const {
	return buffer.data_left() >= p_frames;
}

PackedVector2Array PKAudioEffectCapture::peek_buffer(int p_frames) {
	ERR_FAIL_COND_V(!buffer_initialized, PackedVector2Array());
	ERR_FAIL_INDEX_V(p_frames, buffer.size(), PackedVector2Array());
	int data_left = buffer.data_left();
	if (data_left < p_frames || p_frames == 0) {
		return PackedVector2Array();
	}

	PackedVector2Array ret;
	ret.resize(p_frames);

	Vector<AudioFrame> streaming_data;
	streaming_data.resize(p_frames);
	buffer.peek(streaming_data.ptrw(), p_frames);
	for (int32_t i = 0; i < p_frames; i++) {
		// Godot-cpp doesn't expose the vector proxies to write without CoW. This may be less efficient than the original AudioEffectCapture.
		ret[i] = Vector2(streaming_data[i].left, streaming_data[i].right);
	}
	return ret;
}

void PKAudioEffectCapture::clear_buffer() {
	const int32_t data_left = buffer.data_left();
	buffer.advance_read(data_left);
}

void PKAudioEffectCapture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("can_get_buffer", "frames"), &PKAudioEffectCapture::can_get_buffer);
	ClassDB::bind_method(D_METHOD("clear_buffer"), &PKAudioEffectCapture::clear_buffer);
	ClassDB::bind_method(D_METHOD("get_buffer_length"), &PKAudioEffectCapture::get_buffer_length);
}
