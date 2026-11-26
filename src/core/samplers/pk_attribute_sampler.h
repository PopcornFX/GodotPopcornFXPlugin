//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/resource.hpp"

#include "pk_register_types.h"
#include "scene/pk_emitter_2d.h"
#include "scene/pk_emitter_3d.h"
#include "scene/pk_emitter_control.h"

namespace PopcornFX {
class CResourceDescriptor;
}

namespace godot {

class PKAttributeSampler : public Resource {
	GDCLASS(PKAttributeSampler, Resource);
	friend class PKAttributeList;

public:
	PKAttributeSampler() {
		set_local_to_scene(true);
	}

	virtual const PParticleSamplerDescriptor &get_sampler() { return desc; }
	virtual void set_emitter(Node *p_emitter) { parent = p_emitter; }

protected:
	static void _bind_methods() {}
	virtual void _physics_process() {}
	virtual void _ready() {}
	virtual void _update_from_descriptor(const CResourceDescriptor *p_descriptor) {}
	virtual PackedStringArray _get_configuration_warnings() const { return {}; }

	PParticleSamplerDescriptor desc;

	Node *parent = nullptr;
	PKEmitter3D *get_parent_3d() const { return cast_to<PKEmitter3D>(parent); }
	PKEmitter2D *get_parent_2d() const { return cast_to<PKEmitter2D>(parent); }
	PKEmitterControl *get_parent_control() const { return cast_to<PKEmitterControl>(parent); }
};

} // namespace godot
