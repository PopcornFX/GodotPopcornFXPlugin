//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/editor_plugin.hpp"

#include "integration/pk_sdk.h"

#include <pk_kernel/include/kr_memoryviews.h>
#include <pk_kernel/include/kr_string_id.h>
#include <pk_kernel/include/kr_threads_basics.h>

namespace godot {

class PKEmitter3D;

class PKManager : public Node {
	GDCLASS(PKManager, Node)
public:
	enum { AUDIO_BUFFER_SIZE = 1024 }; // for now, set as the default in popcorn. It seems to be what Unity does aswell (at least, in some parts there, the constant is used as-is)

	struct AudioBufferDescriptor {
		TArray<float, TArrayAligned16> data; // using Popcorn class instead of Godot's LocalVector because they don't have an aligned allocator
		LocalVector<const float *> pyramid;
		uint32_t base_buffer_size = 0;
		bool is_up_to_date = false;
		bool has_been_cleared = false;

		AudioBufferDescriptor(const uint32_t p_base_buffer_size = 0) { resize(p_base_buffer_size); }

		_FORCE_INLINE_ float *data_ptr() { return data.RawDataPointer(); }

		void resize(const uint32_t p_base_buffer_size);
	};

	void init();
	virtual void _process(double p_delta) override;
	virtual void _exit_tree() override;
	void render();

	static const PKManager *get_singleton() { return singleton; }

	TMemoryView<const float *const> get_waveform(CStringId p_channel_group, u32 &r_base_count) const;
	TMemoryView<const float *const> get_spectrum(CStringId p_channel_group, u32 &r_base_count) const;

	~PKManager();

protected:
	static PKManager *singleton;

	static void _bind_methods();
	void _update_audio_buffers();

	void _add_setting_ifn(Variant::Type p_type, const String &p_name, PropertyHint p_hint, const char *p_hint_string, Variant p_default_value, bool p_requires_restart, bool p_is_internal);
	void _update_settings();

	void _integrate_into_engine(bool p_in_editor);
	void _integrate_into_scene_tree();

private:
	const Viewport *_get_viewport() const;
	Threads::CCriticalSection audiosampling_lock;

	// since CStringId is not usable inside a hashmap, we use its underlying numeric ID.
	HashMap<uint32_t, AudioBufferDescriptor> cached_audio_waveform_buffers;
	HashMap<uint32_t, AudioBufferDescriptor> cached_audio_spectrum_buffers;
};

} //namespace godot
