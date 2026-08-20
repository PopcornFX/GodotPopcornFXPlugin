//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/audio_stream.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_player2d.hpp"
#include "godot_cpp/classes/audio_stream_player3d.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/templates/hash_map.hpp"

#include "integration/pk_error_handling.h"

namespace godot {

template <typename AudioStreamPlayerType>
class PKAudioPlayerPool {
public:
	PKAudioPlayerPool() = default;

	PKAudioPlayerPool(Node *p_parent, uint32_t p_size) {
		set_sources_parent(p_parent);
		resize(p_size);
	}

	~PKAudioPlayerPool() {
		// release everything
		for (KeyValue<Vector2i, PKActiveSourceInfo> &entry : active_sources) {
			release_source(entry.key);
		}

		for (AudioStreamPlayerType *ptr : pooled_sources) {
			ptr->queue_free();
		}
		pooled_sources.clear();
		active_sources.clear();
	}

	void set_sources_parent(Node *p_new_parent) {
		if (!PKGD_VERIFY(p_new_parent != nullptr)) {
			return;
		}
		sources_parent = p_new_parent->get_instance_id();

		// update currently active sources
		for (KeyValue<Vector2i, PKActiveSourceInfo> &entry : active_sources) {
			entry.value.player_node->reparent(p_new_parent);
			entry.value.player_node->set_owner(p_new_parent);
		}
	}

	_FORCE_INLINE_ void resize(uint32_t p_new_size) {
		int diff = max_source_count - p_new_size; // diff is positive when we're shrinking

		if (diff == 0) {
			return;
		} else if (diff < 0) { // if we're bigger, just add new sources to the pool
			pooled_sources.reserve(p_new_size);
			for (uint32_t i = max_source_count; i < p_new_size; ++i) {
				AudioStreamPlayerType *player = memnew(AudioStreamPlayerType);
				pooled_sources.push_back(player);
			}

		} else if (diff <= pooled_sources.size()) { // we're smaller but we have enough sources in the pool to keep the active ones untouched

			for (uint32_t i = pooled_sources.size() - diff; i < pooled_sources.size(); ++i) {
				memdelete(pooled_sources[i]);
			}
			pooled_sources.resize(pooled_sources.size() - diff);

		} else { // we're shrinking smaller than currently available sources. We must also remove some active sources.

			uint32_t remainder_to_free = diff - pooled_sources.size();
			for (uint32_t i = 0; i < remainder_to_free; ++i) {
				release_source(active_sources.last()->key);
			}

			for (uint32_t i = 0; i < pooled_sources.size(); ++i) {
				memdelete(pooled_sources[i]);
			}
			pooled_sources.clear();
		}

		max_source_count = p_new_size;
	}

	_FORCE_INLINE_ uint32_t get_max_source_count() {
		return max_source_count;
	}

	struct PKActiveSourceInfo {
		AudioStreamPlayerType *player_node;
		bool updated_this_frame;

		PKActiveSourceInfo() = default;
		PKActiveSourceInfo(AudioStreamPlayerType *p_player_node) :
				player_node(p_player_node), updated_this_frame(false) {}
	};

	uint32_t max_source_count = 0;
	uint64_t sources_parent = 0;

	LocalVector<AudioStreamPlayerType *> pooled_sources;
	HashMap<Vector2i, PKActiveSourceInfo> active_sources;

	PKActiveSourceInfo *spawn_source(const Vector2i p_self_id, const Ref<AudioStream> p_stream, const float p_start_age) {
		Node *sources_parent_ptr = Object::cast_to<Node>(ObjectDB::get_instance(sources_parent));

		const Vector2i self_id = p_self_id;
		PKGD_ASSERT(!active_sources.has(self_id));
		PKGD_ASSERT(sources_parent_ptr != nullptr);

		if (!pooled_sources.is_empty()) {
			const uint32_t idx = pooled_sources.size() - 1;

			AudioStreamPlayerType *src = pooled_sources[idx];
			PKGD_ASSERT(!src->get_parent());

			pooled_sources.remove_at(idx);
			sources_parent_ptr->add_child(src);
			src->set_owner(sources_parent_ptr);
			active_sources[self_id] = src;

			src->set_stream(p_stream);
			src->play(p_start_age);

			return active_sources.getptr(self_id);
		}
		return nullptr;
	}

	_FORCE_INLINE_ void release_source(const Vector2i p_idx) {
		Node *sources_parent_ptr = Object::cast_to<Node>(ObjectDB::get_instance(sources_parent));

		if (sources_parent_ptr) { // Invalid is okay here. It'll be the case at shutdown because the tree is destroyed before us.
			PKGD_ASSERT(active_sources.has(p_idx));
			AudioStreamPlayerType *src = active_sources[p_idx].player_node;
			if (src->is_playing()) {
				src->stop();
			}
			src->set_owner(nullptr);
			sources_parent_ptr->remove_child(src);
			active_sources.erase(p_idx);
			pooled_sources.push_back(src);
		}
	}

	void release_all() {
		for (KeyValue<Vector2i, PKActiveSourceInfo> &kv : active_sources) {
			release_source(kv.key);
		}
	}

	void update_source(AudioStreamPlayerType *p_audioSource, const Vector3 p_position, const float p_doppler_level, const float p_volume);
};

} // namespace godot
