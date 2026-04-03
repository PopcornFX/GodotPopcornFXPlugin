//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/classes/resource_format_loader.hpp"

#include "integration/pk_sdk.h"
#include "pk_particles/include/ps_effect.h"

namespace PopcornFX {
PK_FORWARD_DECLARE(ParticleEffect);
}

namespace godot {

class PKEffect : public Resource {
	GDCLASS(PKEffect, Resource);

public:
	String get_load_path() { return load_path; }
	const PParticleEffect get_effect() { return effect; }
	Error load(const String &p_path);
	PParticleEffectInstance create_instance();

	PKEffect() = default;
	~PKEffect();

protected:
	static void _bind_methods();

private:
	PParticleEffect effect = nullptr;
	String load_path;
};

class ResourceFormatLoaderPKEffect : public ResourceFormatLoader {
	GDCLASS(ResourceFormatLoaderPKEffect, ResourceFormatLoader);

public:
	virtual Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
	virtual PackedStringArray _get_recognized_extensions() const override;
	virtual bool _handles_type(const StringName &p_type) const override;
	virtual String _get_resource_type(const String &p_path) const override;
	virtual PackedStringArray _get_dependencies(const String &p_path, bool p_add_types) const override;

protected:
	static void _bind_methods() {}
};

} // namespace godot
