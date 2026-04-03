//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/audio_effect.hpp"
#include "godot_cpp/classes/audio_effect_instance.hpp"

#include "pk_ring_buffer.h"

namespace godot {

class PKAudioEffectCapture;

class PKAudioEffectCaptureInstance : public AudioEffectInstance {
	GDCLASS(PKAudioEffectCaptureInstance, AudioEffectInstance);
	friend class PKAudioEffectCapture;

public:
	virtual void _process(const void *p_src_frames, AudioFrame *r_frames, int32_t p_frame_count) override;
	virtual bool _process_silence() const override;

protected:
	static void _bind_methods() {}

private:
	Ref<PKAudioEffectCapture> base;
};

class PKAudioEffectCapture : public AudioEffect {
	GDCLASS(PKAudioEffectCapture, AudioEffect);
	friend class PKAudioEffectCaptureInstance;

public:
	PKAudioEffectCapture();

	virtual Ref<AudioEffectInstance> _instantiate() override;

	void set_buffer_length(uint32_t p_buffer_length_samples);
	uint32_t get_buffer_length();
	uint32_t get_frames_left() const;

	bool can_get_buffer(int p_frames) const;
	PackedVector2Array peek_buffer(int p_frames);
	void clear_buffer();

protected:
	static void _bind_methods();

private:
	PopcornFXRingBuffer<AudioFrame> buffer;
	uint32_t buffer_length_samples = 1024u;
	bool buffer_initialized = false;
};

} // namespace godot
