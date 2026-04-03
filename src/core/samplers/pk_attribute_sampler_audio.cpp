//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_attribute_sampler_audio.h"

#include "godot_cpp/classes/audio_stream_playback.hpp"

#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"
#include "pk_manager.h"

namespace godot {

PKAttributeSamplerAudio::PKAttributeSamplerAudio() {
	desc = PK_NEW(CParticleSamplerDescriptor_Audio_Default);

	if (is_spectrum) {
		_register_as_spectrum_sampler();
	} else {
		_register_as_waveform_sampler();
	}
}
PKAttributeSamplerAudio::PKAttributeSamplerAudio(const PopcornFX::CResourceDescriptor *p_resource_desc) {
	desc = PK_NEW(CParticleSamplerDescriptor_Audio_Default);
	const PopcornFX::CResourceDescriptor_Audio *resource_desc = HBO::Cast<const PopcornFX::CResourceDescriptor_Audio>(p_resource_desc);

	target_popcorn_channel_group_string = String(resource_desc->ChannelGroupNameGUID().ToStringData());
	is_spectrum = resource_desc->Mode();

	if (is_spectrum) {
		_register_as_spectrum_sampler();
	} else {
		_register_as_waveform_sampler();
	}
	_update_popcorn_channel_group();
}

PKAttributeSamplerAudio::~PKAttributeSamplerAudio() {
	if (is_spectrum) {
		_unregister_as_spectrum_sampler();
	} else {
		_unregister_as_waveform_sampler();
	}
}

bool PKAttributeSamplerAudio::capture_waveform(float *r_acc, const uint32_t p_n_samples) const {
	switch (source_mode) {
		case BUS_AS_SOURCE:
			return _capture_waveform_from_audio_capture(r_acc, p_n_samples);

		case AUDIOSTREAMPLAYER_AS_SOURCE:
			return _capture_waveform_from_player(r_acc, p_n_samples);

		default:
			return false;
	}
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_source_mode(const int32_t p_mode) {
	source_mode = SourceMode(p_mode);
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_effect_index(int32_t p_idx) {
	effect_index = p_idx;
	_bus_changed();
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_bus_name(StringName p_name) {
	bus_name = p_name;
	_bus_changed();
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_audiostreamplayer(AudioStreamPlayer *p_node) {
	AudioStreamPlayer *old = audio_stream_player;
	audio_stream_player = p_node;
	_audiostreamplayer_changed();
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_is_spectrum(bool p_val) {
	// unregister if already registered
	if (is_spectrum) {
		_unregister_as_spectrum_sampler();
	} else {
		_unregister_as_waveform_sampler();
	}

	is_spectrum = p_val;

	if (is_spectrum) {
		_register_as_spectrum_sampler();
	} else {
		_register_as_waveform_sampler();
	}
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_is_pk_channel_global(bool p_val) {
	if (p_val == is_pk_channel_global) {
		return;
	}
	is_pk_channel_global = p_val;

	_update_popcorn_channel_group();
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::set_target_popcorn_channel_group_string(StringName p_string) {
	target_popcorn_channel_group_string = p_string;
	_update_popcorn_channel_group();
}

void PKAttributeSamplerAudio::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, INT, source_mode, PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, NODE_PATH, audiostreamplayer, PROPERTY_HINT_NODE_TYPE, "AudioStreamPlayer", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, INT, effect_index, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, STRING_NAME, bus_name, PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, STRING_NAME, target_popcorn_channel_group_string, PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, FLOAT, smoothing_factor, PROPERTY_HINT_RANGE, "0, 1", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, FLOAT, scale_factor, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, BOOL, is_spectrum, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerAudio, BOOL, is_pk_channel_global, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
}

void PKAttributeSamplerAudio::_bus_changed() {
	// unref the previously selected audio effect
	audio_capture.unref();

	if (bus_name.is_empty()) {
		return;
	}

	AudioServer *audio_server = AudioServer::get_singleton();
	const int32_t bus_index = audio_server->get_bus_index(bus_name);

	// Check the bus index is valid
	if (bus_index < 0 || bus_index >= audio_server->get_bus_count()) {
		return;
	}
	// Check the effect index is valid
	if (effect_index < 0 || effect_index >= audio_server->get_bus_effect_count(bus_index)) {
		return;
	}

	Ref<AudioEffect> selected_effect = audio_server->get_bus_effect(bus_index, effect_index);
	if (!selected_effect.is_valid() || !selected_effect->is_class(PKAudioEffectCapture::get_class_static())) {
		ERR_FAIL_MSG("The selected audio capture at index" + String::num(effect_index) + "is not a PopcornFxAudioCapture.");
	}

	audio_capture = selected_effect;

	emit_changed();
}

void PKAttributeSamplerAudio::_audiostreamplayer_changed() {
	// Disconnect the last one

	if (audio_stream_player == nullptr) {
		return;
	}
	emit_changed();
}

void PKAttributeSamplerAudio::_update_popcorn_channel_group() {
	if (is_pk_channel_global) {
		target_popcorn_channel_group = CStringId(reinterpret_cast<const char *>(target_popcorn_channel_group_string.to_utf8_buffer().ptr()));
	} else {
		// generate a channel name from the unique id of this object. This can actually collide with user-named channel, but if they put THAT prefix, then respectfully, they asked for it.
		const String rid_string = "@@PrivatePopcornChannelGroup-" + String::num_int64(get_instance_id());
		target_popcorn_channel_group = CStringId(rid_string.utf8().ptr());
	}
	reinterpret_cast<CParticleSamplerDescriptor_Audio_Default *>(desc.Get())->m_ChannelGroupNameID = target_popcorn_channel_group;
}

bool PKAttributeSamplerAudio::_capture_waveform_from_audio_capture(float *r_acc, const uint32_t p_n_samples) const {
	if (audio_capture.is_valid() && audio_capture->can_get_buffer(p_n_samples)) { // if we do have a source

		last_audio_buffer.resize(MAX(last_audio_buffer.size(), p_n_samples));
		PackedVector2Array buffer = audio_capture->peek_buffer(p_n_samples);

		if (buffer.is_empty()) {
			return false;
		}

		// convert the buffer to the right format
		for (uint64_t i = 0; i < p_n_samples; ++i) {
			float val = (buffer[i][0] + buffer[i][1]) * scale_factor; // mono + clip
			val = PKLerp(val, last_audio_buffer[i], smoothing_factor);
			val = (TNumericTraits<float>::IsFinite(val)) ? val : 0.0f;
			r_acc[i] = val;
			last_audio_buffer[i] = r_acc[i];
		}
		return true;
	} else {
		return false;
	}
}

bool PKAttributeSamplerAudio::_capture_waveform_from_player(float *r_acc, const uint32_t p_n_samples) const {
	// TODO This does not work for now. Keeping this for the future.
#if 0
	if (audio_stream_player != nullptr && audio_stream_player->has_stream_playback()) { // if we do have a source

		Ref<AudioSamplePlayback> sample_playback = audio_stream_player->get_stream_playback()->get_sample_playback();
		if (!sample_playback.is_valid()) {
			return false;
		}

		const int64_t current_position = audio_stream_player->get_playback_position() * AudioServer::get_singleton()->get_mix_rate();

		PKGD_ASSERT(sample_playback->volume_vector.size() > current_position);

		// We center our sampling on the player's current position
		const int64_t start_position = current_position - p_n_samples / 2;
		const int64_t end_position = current_position + p_n_samples / 2;

		if (start_position < 0 || end_position > sample_playback->volume_vector.size()) {
			return false;
		}

		const Vector<AudioFrame> &vec = sample_playback->volume_vector;

		// convert the buffer to the right format
		for (int64_t i = start_position; i < end_position; ++i) {
			float val = CLAMP(vec[i].left + vec[i].right, -1.0, 1.0); // mono + clip
			r_acc[i] = val;
		}
		return true;
	} else {
		return false;
	}
#endif
	return false;
}

LocalVector<const PKAttributeSamplerAudio *> PKAttributeSamplerAudio::waveform_samplers = {};

LocalVector<const PKAttributeSamplerAudio *> PKAttributeSamplerAudio::spectrum_samplers = {};

_FORCE_INLINE_ void PKAttributeSamplerAudio::_unregister_as_waveform_sampler() const {
	const int64_t i = waveform_samplers.find(this);
	if (i != -1) {
		waveform_samplers.remove_at_unordered(i);
	}
}

_FORCE_INLINE_ void PKAttributeSamplerAudio::_unregister_as_spectrum_sampler() const {
	const int64_t i = spectrum_samplers.find(this);
	if (i != -1) {
		spectrum_samplers.remove_at_unordered(i);
	}
}

} //namespace godot
