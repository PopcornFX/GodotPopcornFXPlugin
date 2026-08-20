//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_batch_drawer_sound_cpu.h"

#include "godot_cpp/classes/project_settings.hpp"

#include "core/pk_audio_player_pool.h"
#include "integration/engine/pk_file_system_controller.h"
#include "integration/pk_plugin.h"
#include "pk_frame_collector_types.h"
#include "pk_manager.h"
#include "pk_renderer_cache.h"

#include <pk_particles/include/Storage/MainMemory/storage_ram.h>

_FORCE_INLINE_ bool _compare_volume(int32_t p_a, int32_t p_b, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	return p_volumes[p_indices[p_a]] >= p_volumes[p_indices[p_b]];
}

_FORCE_INLINE_ void _selection_sort(int32_t p_first, int32_t p_last, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	while (--p_last > p_first) {
		int32_t max = p_last;
		for (int32_t p = p_last; p > p_first;) {
			--p;
			if (_compare_volume(p, max, p_indices, p_volumes)) {
				continue;
			}
			max = p;
		}
		if (max != p_last) {
			PKSwap(p_indices[max], p_indices[p_last]);
		}
	}
}

_FORCE_INLINE_ int32_t _quick_select_partition(int32_t p_left, int32_t p_right, int32_t p_pivot, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	const int32_t pivot_ptr = p_right;
	int32_t store_ptr = p_left;

	PKSwap(p_indices[p_pivot], p_indices[p_right]);
	for (int32_t it = p_left; it < p_right; ++it) {
		if (_compare_volume(it, pivot_ptr, p_indices, p_volumes)) {
			PKSwap(p_indices[store_ptr], p_indices[it]);
			++store_ptr;
		}
	}
	PKSwap(p_indices[p_right], p_indices[store_ptr]);
	return store_ptr;
}

_FORCE_INLINE_ void _quick_select_impl(int32_t p_left, int32_t p_right, int32_t p_n, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	int32_t right = p_right - 1;
	int32_t left = p_left;
	while (right - left >= 3) {
		const uintptr_t half = uintptr_t(right - left) / 2;
		int32_t pivot_ptr = right - half;
		pivot_ptr = _quick_select_partition(left, right, pivot_ptr, p_indices, p_volumes);
		if (p_n == pivot_ptr) {
			return;
		} else if (p_n < pivot_ptr) {
			right = pivot_ptr - 1;
		} else {
			left = pivot_ptr + 1;
		}
	}
	_selection_sort(left, right + 1, p_indices, p_volumes);
}

_FORCE_INLINE_ void _quick_select_range_impl(int32_t p_left, int32_t p_right, int32_t p_begin_nth, int32_t p_end_nth, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	--p_end_nth;
	u32 dist_from_left = u32(p_begin_nth - p_left);
	u32 dist_from_right = u32(p_right - p_end_nth);

	if (dist_from_left > dist_from_right) {
		_quick_select_impl(p_left, p_right, p_begin_nth, p_indices, p_volumes);
		if (p_begin_nth != p_end_nth) {
			_quick_select_impl(p_begin_nth + 1, p_right, p_end_nth, p_indices, p_volumes);
		}
	} else {
		_quick_select_impl(p_left, p_right, p_end_nth, p_indices, p_volumes);
		if (p_begin_nth != p_end_nth) {
			_quick_select_impl(p_left, p_end_nth, p_begin_nth, p_indices, p_volumes);
		}
	}
}

_FORCE_INLINE_ void quick_select_range(int32_t p_first, int32_t p_last, int32_t p_begin_nth, int32_t p_end_nth, LocalVector<uint32_t> &p_indices, LocalVector<float> &p_volumes) {
	// The last is inclusive
	if ((p_last - p_first > 0) && (p_end_nth - p_begin_nth > 0)) {
		_quick_select_range_impl(p_first, p_last, p_begin_nth, p_end_nth, p_indices, p_volumes);
	}
}

void select_biggest_volumes(LocalVector<uint32_t> &p_indices, uint32_t p_n, LocalVector<float> &p_volumes) {
	quick_select_range(0, p_indices.size() - 1, 0, p_n, p_indices, p_volumes);
}

PKBatchDrawerSoundCPU::PKBatchDrawerSoundCPU() {
}

PKBatchDrawerSoundCPU::~PKBatchDrawerSoundCPU() {
}

bool PKBatchDrawerSoundCPU::Setup(const CRendererDataBase *p_renderer, const CParticleRenderMedium *p_owner, const CFrameCollector *p_fc, const CStringId &p_storage_class) {
	if (!Super::Setup(p_renderer, p_owner, p_fc, p_storage_class)) {
		return false;
	}

	owner_scene = PKPlugin::get_singleton()->get_scene();
	if (!PKGD_VERIFY(owner_scene != nullptr)) {
		return false;
	}

	return true;
}

bool PKBatchDrawerSoundCPU::AreRenderersCompatible(const CRendererDataBase *p_renderer_a, const CRendererDataBase *p_renderer_b) const {
	if (!Super::AreRenderersCompatible(p_renderer_a, p_renderer_b)) {
		return false;
	}
	return false;
}

bool PKBatchDrawerSoundCPU::CanRender(SRenderContext &p_ctx) const {
	if (!Super::CanRender(p_ctx)) {
		return false;
	}
	return true;
}

void PKBatchDrawerSoundCPU::BeginFrame(SRenderContext &p_ctx) {
}

bool PKBatchDrawerSoundCPU::EmitDrawCall(SRenderContext &p_ctx, const SDrawCallDesc &p_to_emit) {
	for (uint32_t i = 0; i < p_to_emit.m_DrawRequests.Count(); ++i) {
		const Drawers::SSound_DrawRequest *soundRequest = static_cast<const Drawers::SSound_DrawRequest *>(p_to_emit.m_DrawRequests[i]);
		if (soundRequest != null) {
			PKRendererCacheAudio *renderer_cache = static_cast<PKRendererCacheAudio *>(p_to_emit.m_RendererCaches[i].Get());
			if (!PKGD_VERIFY(renderer_cache != null)) {
				return false;
			}
			return _render_audio(soundRequest, renderer_cache, p_to_emit.m_TotalParticleCount);
		}
	}
	return false;
}

bool PKBatchDrawerSoundCPU::_render_audio(const Drawers::SSound_DrawRequest *p_sound_request, PKRendererCacheAudio *p_renderer_cache, const u32 p_particle_count) {
	PKManager *manager = PKManager::get_singleton();
	const String &sound_path = p_renderer_cache->material_desc_sound.sound_path;
	const Ref<AudioStream> stream = PKFileSystemController::get_default()->load_resource_from_virtual_path<AudioStream>(sound_path);
	if (stream.is_null()) {
		return false;
	}
	PKAudioPlayerPool<AudioStreamPlayer3D> &pool = p_renderer_cache->material_desc_sound.pool; // TODO modify when 2D

	const uint32_t max_audio_source_count = ProjectSettings::get_singleton()->get_setting("popcornfx/runtime/max_audio_sources_count_per_clip");
	pool.resize(max_audio_source_count);

	if (pool.sources_parent == 0) {
		return false;
	}

	const u32 draw_request_particle_count = p_sound_request->RenderedParticleCount();
	PK_ASSERT(!p_sound_request->Empty());
	PK_ASSERT(draw_request_particle_count > 0);

	const Drawers::SSound_BillboardingRequest &bb_request = static_cast<const Drawers::SSound_BillboardingRequest &>(p_sound_request->BaseBillboardingRequest());
	if (p_sound_request->StorageClass() != CParticleStorageManager_MainMemory::DefaultStorageClass()) {
		PK_ASSERT_NOT_REACHED();
		return false;
	}
	const CParticleStreamToRender_MainMemory *locked_stream = p_sound_request->StreamToRender_MainMemory();
	if (!PK_VERIFY(locked_stream != null)) {
		return false;
	}

	using PKActiveSourceInfo3D = PKAudioPlayerPool<AudioStreamPlayer3D>::PKActiveSourceInfo;

	for (KeyValue<Vector2i, PKActiveSourceInfo3D> &entry : pool.active_sources) {
		entry.value.updated_this_frame = false;
	}

	const uint32_t page_count = locked_stream->PageCount();
	for (uint32_t pagei = 0; pagei < page_count; ++pagei) {
		const CParticlePageToRender_MainMemory *page = locked_stream->Page(pagei);
		PK_ASSERT(page != null && !page->Empty());
		const u32 pcount = page->InputParticleCount();
		const float doppler_factor = bb_request.m_DopplerFactor;

		TStridedMemoryView<const Vector2i> self_ids = TStridedMemoryView<const Vector2i>(page->StreamForReading<CInt2>(bb_request.m_SelfIDStreamId));
		TStridedMemoryView<const Vector3> positions = TStridedMemoryView<const Vector3>(page->StreamForReading<CFloat3>(bb_request.m_PositionStreamId));
		TStridedMemoryView<const float> life_ratios = TStridedMemoryView<const float>(page->StreamForReading<float>(bb_request.m_LifeRatioStreamId));
		TStridedMemoryView<const float> inv_lives = TStridedMemoryView<const float>(page->StreamForReading<float>(bb_request.m_InvLifeStreamId));
		TStridedMemoryView<const float> volumes = bb_request.m_VolumeStreamId.Valid() ? page->StreamForReading<float>(bb_request.m_VolumeStreamId) : TStridedMemoryView<const float>(&CFloat4::ONE.x(), positions.Count(), 0);

		const u8 enabledTrue = 0xFF;
		TStridedMemoryView<const uint8_t> enabled_particles = (bb_request.m_EnabledStreamId.Valid()) ? page->StreamForReading<bool>(bb_request.m_EnabledStreamId) : TStridedMemoryView<const u8>(&enabledTrue, pcount, 0);

		const uint32_t pos_count = positions.Count();
		PK_ASSERT(
				pos_count == pcount &&
				pos_count == self_ids.Count() &&
				pos_count == volumes.Count());

		// Dispatch AudioStreamPlayers.

		// First get the biggest contributors from p_sound_infos.

		uint32_t max_attributable_sources = MIN(pool.pooled_sources.size(), pcount);

		// We do the quickselect in-place on an index permutation array (static so it doesn't re-alloc each frame).
		static LocalVector<uint32_t> index_array;
		static LocalVector<float> volumes_attenuated;

		const bool need_selecting = pcount > max_attributable_sources;

		if (need_selecting) {
			index_array.resize(pcount);
			volumes_attenuated.resize(pcount);

			Vector3 listener_position = manager->get_current_audio_listener_3D()->get_global_position();

			for (int i = 0; i < pcount; ++i) {
				index_array[i] = i;
				float attenuation = 1.0 / positions[i].distance_squared_to(listener_position);
				volumes_attenuated[i] = volumes[i] * attenuation;
			}

			select_biggest_volumes(index_array, max_attributable_sources, volumes_attenuated);
		}

		// Sanity check.
		for (int i = 0; i < max_attributable_sources; ++i) {
			for (int j = max_attributable_sources; j < pcount; ++j) {
				PKGD_ASSERT_MSG(volumes[index_array[i]] >= volumes[index_array[j]], "Selecting of biggest contributing sounds failed.");
			}
		}

		for (uint32_t i = 0; i < pcount; ++i) {
			const uint32_t idx = need_selecting ? index_array[i] : i; // Don't use the permutation if we don't need selecting.

			if (!enabled_particles[idx]) {
				continue;
			}

			const Vector2i self_id = self_ids[idx];
			const Vector3 position = positions[idx];
			const float volume = volumes[idx];
			const float age = life_ratios[idx] / inv_lives[idx];

			// Get the corresponding source (or take a new one from the pool if needed).
			PKActiveSourceInfo3D *source;
			if (pool.active_sources.has(self_id)) {
				source = pool.active_sources.getptr(self_id);
			} else {
				source = pool.spawn_source(self_id, stream, age);
			}
			// Check if a source was available.
			if (source == nullptr) {
				break; // no sources anymore
			}

			pool.update_source(source->player_node, position, doppler_factor, volume);
			source->updated_this_frame = true;
		}
	}

	// Release the sources for which we didn't have an update.
	for (const KeyValue<Vector2i, PKActiveSourceInfo3D> &entry : pool.active_sources) {
		if (!entry.value.updated_this_frame) {
			pool.release_source(entry.key);
		}
	}
	owner_scene->set_sound_instance_rendered(&pool);

	return true;
}
