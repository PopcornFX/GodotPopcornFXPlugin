//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_manager.h"

#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/main_loop.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/error_macros.hpp"

#ifdef TOOLS_ENABLED
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/sub_viewport.hpp"
#endif

#include "core/pk_dft.h"
#include "core/samplers/pk_attribute_sampler_audio.h"
#include "editor/inspect/pk_inspector_plugin.h"
#include "integration/internal/pk_project_settings.h"
#include "integration/internal/pk_scene.h"
#include "integration/pk_plugin.h"

using namespace godot;

void PKManager::init() {
	// Draw everything after calls from scripts to avoid lagging
	set_process_priority(INT32_MAX);

	Engine *engine = Engine::get_singleton();
	const bool in_editor = Engine::get_singleton()->is_editor_hint();
	engine->register_singleton("PKManager", this);
	if (!PKPlugin::get_singleton()->startup()) {
		return;
	}

	_update_settings();

	// In the editor, we need to delay the initialization of the plugin to let Godot init everything necessary first
	if (in_editor) {
		call_deferred("_integrate_into_engine", in_editor);
	}

	// Without the editor, we need to initialize the plugin as soon as possible
	else {
		_integrate_into_engine(in_editor);
	}
}

PKManager *PKManager::singleton = nullptr;

void PKManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_integrate_into_engine", "in_editor"), &PKManager::_integrate_into_engine);
	ClassDB::bind_method(D_METHOD("_integrate_into_scene_tree"), &PKManager::_integrate_into_scene_tree);
	ClassDB::bind_method(D_METHOD("_render"), &PKManager::render);
}

void PKManager::_integrate_into_engine(bool p_in_editor) {
	if (!singleton) {
		singleton = this;
	} else {
		ERR_FAIL_COND_MSG(false, "Only 1 instance of PopcornFXManager is allowed");
	}

	// In the editor, we can integrate into the scene tree right away, as this function was already a delayed call
	if (p_in_editor) {
		_integrate_into_scene_tree();
	}

	// Without the editor, we need to wait for the scene tree to be initialized, because this function was called as early as possible
	else {
		call_deferred("_integrate_into_scene_tree");
	}
}

void PKManager::_integrate_into_scene_tree() {
	const SceneTree *scene_tree = cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	// Not safe but we'll use it safely (promise)
	PKPlugin::get_singleton()->get_scene()->state = scene_tree->get_root()->get_world_3d()->get_direct_space_state();
	Window *scene_root = scene_tree->get_root();

	scene_root->add_child(this);
	scene_root->move_child(this, 0);
	set_process(true);
}

const Viewport *PKManager::_get_viewport() const {
	const Viewport *vp = nullptr;
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		vp = EditorInterface::get_singleton()->get_editor_viewport_3d();
	} else
#endif
		vp = get_viewport();
	ERR_FAIL_COND_V(vp == nullptr, nullptr);
	return vp;
}

// Audio functions

TMemoryView<const float *const> PKManager::get_waveform(CStringId p_channel_group, u32 &r_base_count) const {
	const uint32_t id = p_channel_group.Id();

	PK_SCOPEDLOCK(audiosampling_lock);
	if (cached_audio_waveform_buffers.has(id)) {
		const AudioBufferDescriptor &desc = cached_audio_waveform_buffers.get(id);

		if (!desc.is_up_to_date) {
			r_base_count = 0;
			return TMemoryView<const float *const>();
		}
		r_base_count = AUDIO_BUFFER_SIZE;
		return TMemoryView<const float *const>(desc.pyramid.ptr(), desc.pyramid.size());
	}
	// if not found (should not happen normally after the first few init frames)
	r_base_count = 0;
	return TMemoryView<const float *const>();
}

TMemoryView<const float *const> PKManager::get_spectrum(CStringId p_channel_group, u32 &r_base_count) const {
	const uint32_t id = p_channel_group.Id();

	PK_SCOPEDLOCK(audiosampling_lock);
	if (cached_audio_spectrum_buffers.has(id)) {
		const AudioBufferDescriptor &desc = cached_audio_spectrum_buffers.get(id);

		if (!desc.is_up_to_date) {
			r_base_count = 0;
			return TMemoryView<const float *const>();
		}
		r_base_count = AUDIO_BUFFER_SIZE;
		return TMemoryView<const float *const>(desc.pyramid.ptr(), desc.pyramid.size());
	}
	// if not found (should not happen normally after the first few init frames)
	r_base_count = 0;
	return TMemoryView<const float *const>();
}

template <typename T> // Godot's version is NOT a floor_log2.
constexpr T true_floor_log2(T p_x) {
	return p_x <= 2 ? p_x - 1 : 1 + true_floor_log2(p_x >> 1);
}

static constexpr uint32_t get_pyramid_height(const uint32_t p_first_level_size) {
	return true_floor_log2(p_first_level_size) + 1;
}

// How many float are in the pyramid in total.
static constexpr uint32_t get_total_pyramid_size(const uint32_t p_first_level_size) {
	return (p_first_level_size * 2 - 1) + 4 * get_pyramid_height(p_first_level_size); // count of total pyramid elements, plus the borders
}

// This assumes that the buffer is allocated with the right size for the whole pyramid, including borders.
// And that the buffer already contains the base data, including its borders.
static bool build_audio_pyramid(float *p_buffer, LocalVector<const float *> &p_pyramid, const uint32_t p_base_buffer_size) {
	PKGD_ASSERT(p_buffer != nullptr);
	PKGD_ASSERT(is_power_of_2(p_base_buffer_size));

	const uint32_t pyramid_height = get_pyramid_height(p_base_buffer_size);
	p_pyramid.resize(pyramid_height);
	p_pyramid[0] = p_buffer;

	float *last_layer_ptr = p_buffer;
	uint32_t current_count = p_base_buffer_size;
	for (uint32_t i = 1; i < pyramid_height; i++) {
		// Pointers to the beginning of the data, just after the border
		const float *__restrict src = 2 + last_layer_ptr;
		float *__restrict dst = 2 + last_layer_ptr + current_count + 2 + 2; // using the last current_count to advance to the right place

		current_count >>= 1;

		p_pyramid[i] = dst - 2;

		// downsample
		for (uint32_t j = 0; j < current_count; j++) {
			dst[j] = 0.5f * (src[j * 2 + 0] + src[j * 2 + 1]);
		}
		const float first_entry = dst[0];
		const float last_entry = dst[current_count - 1];

		// Fill the borders with the corresponding values
		dst[-1] = first_entry;
		dst[-2] = first_entry;
		dst[current_count + 0] = last_entry;
		dst[current_count + 1] = last_entry;
		last_layer_ptr = dst - 2;
	}

	return true;
}

_FORCE_INLINE_ void PKManager::AudioBufferDescriptor::resize(const uint32_t p_base_buffer_size) {
	const uint32_t pyamid_height = get_pyramid_height(p_base_buffer_size);
	const uint32_t pyamid_total_size = get_total_pyramid_size(p_base_buffer_size);

	data.Resize(pyamid_total_size);
	pyramid.resize(pyamid_height);
}

void PKManager::_process(double p_delta) {
	const Viewport *vp = _get_viewport();

	const Camera3D *camera = vp->get_camera_3d();
	if (camera == nullptr) {
		return;
	}

	PKPlugin::get_singleton()->update(p_delta, camera->get_camera_transform().inverse(), camera->get_camera_projection());
	RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this, &PKManager::render));

	// update the audio buffers each frame
	_update_audio_buffers();
}

void PKManager::_exit_tree() {
	Engine::get_singleton()->unregister_singleton("PKManager");
	PKPlugin::get_singleton()->shutdown();
}

void PKManager::render() {
	const Viewport *vp = _get_viewport();

	const Ref<World3D> world3d = vp->find_world_3d();
	ERR_FAIL_COND(world3d.is_null());

	const RID scenario_id = world3d->get_scenario();
	ERR_FAIL_COND(!scenario_id.is_valid());

	PKPlugin::get_singleton()->render(scenario_id);
}

PKManager::~PKManager() {
	if (singleton) {
		if (singleton == this) {
			singleton = nullptr;
		} else {
			ERR_PRINT("More than 1 PopcornFXManager instance was created");
		}
	}
}

void PKManager::_update_audio_buffers() {
	PK_SCOPEDLOCK(audiosampling_lock);

	constexpr uint32_t pyramid_size = get_total_pyramid_size(AUDIO_BUFFER_SIZE);

	// Waveform buffers

	for (KeyValue<uint32_t, AudioBufferDescriptor> &v : cached_audio_waveform_buffers) {
		v.value.is_up_to_date = false;
		v.value.has_been_cleared = false;
	}

	const uint32_t waveform_sampler_count = PKAttributeSamplerAudio::get_waveform_sampler_count();
	// We iterate on the samplers, and ask them to fill the buffer using their data source.
	// TODO One problem with that is that if two instances have the same effect, the audio will be added twice to the channel. I think it makes sense this way, but it may be annoying to use. To discuss
	for (uint32_t i = 0; i < waveform_sampler_count; ++i) {
		const PKAttributeSamplerAudio *sampler = PKAttributeSamplerAudio::get_waveform_sampler(i);

		// Lazily create the buffer
		const uint32_t id = sampler->get_target_popcorn_channel_group().Id();
		if (!cached_audio_waveform_buffers.has(id)) {
			cached_audio_waveform_buffers.insert(id, AudioBufferDescriptor(AUDIO_BUFFER_SIZE));
		}
		AudioBufferDescriptor &desc = cached_audio_waveform_buffers.get(id);

		// Lazily clear the first layer of the pyramid (because it's used as an accumulator. The rest will be overwritten anyway)
		if (!desc.has_been_cleared) {
			memset(desc.data_ptr(), 0, (AUDIO_BUFFER_SIZE + 4) * sizeof(float));
			desc.has_been_cleared = true;
		}

		if (sampler->capture_waveform(desc.data_ptr() + 2, AUDIO_BUFFER_SIZE)) {
			desc.is_up_to_date = true;
		}
	}

	// Once we have accumulated all the audio that we need, we can build the pyramid.
	for (KeyValue<uint32_t, AudioBufferDescriptor> &v : cached_audio_waveform_buffers) {
		AudioBufferDescriptor &desc = v.value;

		if (desc.is_up_to_date) {
			float *data_ptr = desc.data_ptr();

			// Sanity check: make sure the values are real numbers (+4 because of the borders)
			for (u32 i = 0; i < AUDIO_BUFFER_SIZE + 4; ++i) {
				PKGD_ASSERT(TNumericTraits<float>::IsFinite(data_ptr[i]));
			}

			// perform clipping
			for (uint32_t i = 2; i < AUDIO_BUFFER_SIZE + 2; ++i) {
				data_ptr[i] = CLAMP(data_ptr[i], 0.0f, 1.0f);
			}

			// set the 2 border values on each side
			data_ptr[0] = data_ptr[2];
			data_ptr[1] = data_ptr[2];
			data_ptr[AUDIO_BUFFER_SIZE + 2 - 1 + 1] = data_ptr[AUDIO_BUFFER_SIZE + 2 - 1];
			data_ptr[AUDIO_BUFFER_SIZE + 2 - 1 + 2] = data_ptr[AUDIO_BUFFER_SIZE + 2 - 1];

			build_audio_pyramid(data_ptr, desc.pyramid, AUDIO_BUFFER_SIZE);

		} else { // Remove the buffers we didnt use
			cached_audio_waveform_buffers.erase(v.key);
		}
	}

	// Spectrum buffers (same circus)

	for (KeyValue<uint32_t, AudioBufferDescriptor> &v : cached_audio_spectrum_buffers) {
		v.value.is_up_to_date = false;
		v.value.has_been_cleared = false;
	}

	const uint32_t spectrum_sampler_count = PKAttributeSamplerAudio::get_spectrum_sampler_count();
	// We iterate on the samplers, and ask them to fill the buffer using their data source.
	for (uint32_t i = 0; i < spectrum_sampler_count; ++i) {
		const PKAttributeSamplerAudio *sampler = PKAttributeSamplerAudio::get_spectrum_sampler(i);

		// Lazily create the buffer
		const uint32_t id = sampler->get_target_popcorn_channel_group().Id();
		if (!cached_audio_spectrum_buffers.has(id)) {
			cached_audio_spectrum_buffers.insert(id, AudioBufferDescriptor(AUDIO_BUFFER_SIZE));
		}
		AudioBufferDescriptor &desc = cached_audio_spectrum_buffers.get(id);

		// Lazily clear the first layer of the pyramid (because it's used as an accumulator. The rest will be overwritten anyway)
		if (!desc.has_been_cleared) {
			memset(desc.data_ptr(), 0, (AUDIO_BUFFER_SIZE + 4) * sizeof(float));
			desc.has_been_cleared = true;
		}

		// Finally, write up the data inside of the first layer (with an offset for the border)
		// We get the waveforms. We'll do the fft on the sum.
		if (sampler->capture_waveform(desc.data_ptr() + 2, AUDIO_BUFFER_SIZE)) {
			// At least one sampler feeds into it, so set it to updated so the get_ functions know they can take this one
			desc.is_up_to_date = true;
		}
	}

	// Once we have accumulated all the audio that we need, we can build the pyramid. Remove the buffers we didnt use
	for (KeyValue<uint32_t, AudioBufferDescriptor> &v : cached_audio_spectrum_buffers) {
		AudioBufferDescriptor &desc = v.value;
		// if this desc has data
		if (desc.is_up_to_date) {
			float *data_ptr = desc.data_ptr();

			// Sanity check: make sure the values are real numbers
			for (u32 i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
				PKGD_ASSERT(TNumericTraits<float>::IsFinite(data_ptr[i]));
			}

			// +2 because we want the data without the border
			PKDiscreteFourierTransform::apply_dft(data_ptr + 2, AUDIO_BUFFER_SIZE);

			// set the borders
			data_ptr[0] = data_ptr[2];
			data_ptr[1] = data_ptr[2];
			data_ptr[AUDIO_BUFFER_SIZE + 2 - 1 + 1] = data_ptr[AUDIO_BUFFER_SIZE + 2 - 1];
			data_ptr[AUDIO_BUFFER_SIZE + 2 - 1 + 2] = data_ptr[AUDIO_BUFFER_SIZE + 2 - 1];

			build_audio_pyramid(data_ptr, desc.pyramid, AUDIO_BUFFER_SIZE);
		}
	}
}

void PKManager::_add_setting_ifn(Variant::Type p_type, const String &p_name, PropertyHint p_hint, const char *p_hint_string, Variant p_default_value, bool p_requires_restart, bool p_is_internal) {
	ProjectSettings *settings = ProjectSettings::get_singleton();
	if (!settings->has_setting(p_name)) {
		settings->set_setting(p_name, p_default_value);
	}
	Dictionary d;
	d["name"] = p_name;
	d["type"] = p_type;
	d["hint"] = p_hint;
	d["hint_string"] = String(p_hint_string);
	settings->add_property_info(d);
	settings->set_initial_value(p_name, p_default_value);
	settings->set_restart_if_changed(p_name, p_requires_restart);
	settings->set_as_internal(p_name, p_is_internal);
	settings->set_as_basic(p_name, true);
}

void PKManager::_update_settings() {
#if TOOLS_ENABLED
	_add_setting_ifn(Variant::INT, "popcornfx/runtime/default_emitter_transform_mode", PROPERTY_HINT_ENUM, "Global,Local", 0, true, false);
	_add_setting_ifn(Variant::STRING, "popcornfx/editor/source_pack", PROPERTY_HINT_FILE, "*.pkproj", "res://", true, false);
	_add_setting_ifn(Variant::BOOL, "popcornfx/editor/debug_baked_effects", PROPERTY_HINT_NONE, "", false, true, false);
	_add_setting_ifn(Variant::BOOL, "popcornfx/export/raw_export_dependencies", PROPERTY_HINT_NONE, "", true, false, false);
	_add_setting_ifn(Variant::STRING, "popcornfx/internal/source_pack_root", PROPERTY_HINT_NONE, "", "", false, true);
#endif

	ProjectSettings *settings = ProjectSettings::get_singleton();
	PKPlugin *pkfx_plugin = PKPlugin::get_singleton();
	SPopcornFXProjectSettings pkfx_settings;
#if TOOLS_ENABLED
	pkfx_settings.source_pack = settings->get_setting("popcornfx/editor/source_pack");
	pkfx_settings.debug_baked_effects = settings->get_setting("popcornfx/editor/debug_baked_effects");
#endif
	pkfx_settings.source_pack_root = settings->get_setting("popcornfx/internal/source_pack_root");

	pkfx_plugin->set_project_settings(pkfx_settings);

	PKEmitter3D::default_transform_mode = PKEmitter3D::TransformMode(int(settings->get_setting("popcornfx/runtime/default_emitter_transform_mode")) + 1);

	// Set this hidden setting so that in a packaged build we know where the effects are
#if TOOLS_ENABLED
	settings->set_setting("popcornfx/internal/source_pack_root", to_gd(pkfx_plugin->get_processed_project_settings()->get_source_pack_root_dir()));
#endif
}
