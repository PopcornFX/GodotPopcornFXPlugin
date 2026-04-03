//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/audio_effect_capture.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/local_vector.hpp"

#include "integration/pk_sdk.h"

#include "core/pk_audio_effect_capture.h"
#include "pk_attribute_sampler.h"

#include <pk_particles/include/ps_samplers_audio.h>

namespace godot {
class PKEmitterPropertiesEditor;
class PKAttributeSamplerAudio : public PKAttributeSampler {
	GDCLASS(PKAttributeSamplerAudio, PKAttributeSampler);
	friend PKEmitterPropertiesEditor; // Only one who must call update_target_data.

public:
	enum SourceMode {
		BUS_AS_SOURCE,
		AUDIOSTREAMPLAYER_AS_SOURCE
	};

	PKAttributeSamplerAudio();
	PKAttributeSamplerAudio(const PopcornFX::CResourceDescriptor *p_resource_desc);
	~PKAttributeSamplerAudio();

	static _FORCE_INLINE_ const PKAttributeSamplerAudio *get_waveform_sampler(const uint32_t p_n) { return waveform_samplers[p_n]; }
	static _FORCE_INLINE_ uint32_t get_waveform_sampler_count() { return waveform_samplers.size(); }

	static _FORCE_INLINE_ const PKAttributeSamplerAudio *get_spectrum_sampler(const uint32_t p_n) { return spectrum_samplers[p_n]; }
	static _FORCE_INLINE_ uint32_t get_spectrum_sampler_count() { return spectrum_samplers.size(); }

	bool capture_waveform(float *r_dst, const uint32_t p_n_samples) const;

	void set_source_mode(const int32_t p_mode); // This can't be enum-typed since godot serializes it as int
	int32_t get_source_mode() const { return source_mode; }

	void set_effect_index(int32_t p_idx);
	int32_t get_effect_index() const { return effect_index; }

	void set_bus_name(const StringName p_name);
	StringName get_bus_name() const { return bus_name; }

	void set_smoothing_factor(const float p_val) { smoothing_factor = p_val; }
	float get_smoothing_factor() const { return smoothing_factor; }

	void set_scale_factor(const float p_val) { scale_factor = p_val; }
	float get_scale_factor() const { return scale_factor; }

	AudioStreamPlayer *get_audiostreamplayer() const { return audio_stream_player; }
	void set_audiostreamplayer(AudioStreamPlayer *p_node);

	CStringId get_target_popcorn_channel_group() const { return target_popcorn_channel_group; }

	bool get_is_spectrum() const { return is_spectrum; }
	void set_is_spectrum(bool p_val);

	bool get_is_pk_channel_global() const { return is_pk_channel_global; }
	void set_is_pk_channel_global(bool p_val);

	void set_target_popcorn_channel_group_string(StringName p_string);
	StringName get_target_popcorn_channel_group_string() { return target_popcorn_channel_group_string; }

protected:
	static void _bind_methods();

	void _bus_changed();
	void _audiostreamplayer_changed();

	void _set_target_popcorn_channel_group(CStringId p_id) { target_popcorn_channel_group = p_id; }
	void _update_popcorn_channel_group();

	bool _capture_waveform_from_audio_capture(float *r_dst, const uint32_t p_n_samples) const;
	bool _capture_waveform_from_player(float *r_dst, const uint32_t p_n_samples) const;

	mutable LocalVector<float> last_audio_buffer; // used for smoothing

	// Note : Audiostream mode is nearly implemented, but it seems godot does not give us a nice way to get data from an audiostreamplayer
	// Once we find a solution, just modify the method capture_waveform_from_audioeffect() and revive the code in emitter_properties_editor to put the switch in the inspector (serialization of the nodepath may need work too)
	SourceMode source_mode = BUS_AS_SOURCE;
	int32_t effect_index = 0;
	StringName bus_name = "";

	StringName target_popcorn_channel_group_string;
	CStringId target_popcorn_channel_group;
	bool is_spectrum = false; // false if waveform
	bool is_pk_channel_global = true;

	float smoothing_factor = 0.0;
	float scale_factor = 1.0;

	AudioStreamPlayer *audio_stream_player = nullptr;
	Ref<PKAudioEffectCapture> audio_capture = nullptr;

private:
	static LocalVector<const PKAttributeSamplerAudio *> waveform_samplers;
	static LocalVector<const PKAttributeSamplerAudio *> spectrum_samplers;

	void _register_as_waveform_sampler() const { waveform_samplers.push_back(this); }
	void _register_as_spectrum_sampler() const { spectrum_samplers.push_back(this); }
	void _unregister_as_waveform_sampler() const;
	void _unregister_as_spectrum_sampler() const;
};

} // namespace godot
