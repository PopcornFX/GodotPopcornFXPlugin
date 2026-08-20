//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_audio_player_pool.h"

using namespace godot;

template<>
void PKAudioPlayerPool<AudioStreamPlayer3D>::update_source(AudioStreamPlayer3D *p_audioSource, const Vector3 p_position, const float p_doppler_level, const float p_volume) {
	p_audioSource->set_global_position(p_position);
	p_audioSource->set_volume_linear(p_volume);
	// Godot doesnt have doppler scaling so we're just disabling it if 0
	p_audioSource->set_doppler_tracking(p_doppler_level > 0 ? AudioStreamPlayer3D::DOPPLER_TRACKING_IDLE_STEP : AudioStreamPlayer3D::DOPPLER_TRACKING_DISABLED);
	//p_audioSource->set_max_distance(p_info.radius); // Not sure what this represents but it's too small to be used as a max distance for godot anyway.
}

template<>
void PKAudioPlayerPool<AudioStreamPlayer2D>::update_source(AudioStreamPlayer2D *p_audioSource, const Vector3 p_position, const float p_doppler_level, const float p_volume) {
	// TODO scale this once we have a scale somewhere.
	const Vector2 pos(p_position.x, p_position.y);
	p_audioSource->set_global_position(pos);
	p_audioSource->set_volume_linear(p_volume);
	// Godot does not have doppler tracking for 2d sounds.
}
