//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/node3d.hpp"

#include "integration/pk_sdk.h"

#include <pk_particles/include/ps_effect.h>

namespace godot {

class PKAttributeList;
class PKEffect;

class PKEmitter3D : public Node3D {
	GDCLASS(PKEmitter3D, Node3D);

public:
	virtual void _physics_process(double p_delta) override;
	virtual void _ready() override;

	const PParticleEffectInstance get_effect_instance() const;
	SSpawnTransformsPack get_transform_pack() const;

	void set_effect(const Ref<PKEffect> &p_effect);
	Ref<PKEffect> get_effect() const;

	void set_effect_is_playing(const bool p_is_playing);
	bool get_effect_is_playing() const;
	bool start_effect();
	void kill_effect();

	void set_attribute_list(Ref<PKAttributeList> p_attribute_list);
	Ref<PKAttributeList> get_attribute_list() const;

	PKEmitter3D();
	~PKEmitter3D();

protected:
	static void _bind_methods();
	void _notification(int32_t p_what);
	friend class PKAttributeList;

private:
	Ref<PKAttributeList> attribute_list;
	Ref<PKEffect> effect;
	PParticleEffectInstance effect_instance;
	bool is_playing;
	bool is_disabled;
	double time_passed;
	CFloat4x4 effect_transform;
	CFloat4x4 effect_prev_transform;
	CFloat3 effect_velocity;
	CFloat3 effect_prev_velocity;

	void _effect_changed();
	void _update_transforms();
	void _reset_transforms();
};

} // namespace godot
