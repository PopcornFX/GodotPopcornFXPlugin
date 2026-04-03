//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/resource.hpp"

#include "pk_register_types.h"
#include "scene/pk_emitter_3d.h"

namespace godot {

class PKAttributeSampler : public Resource {
	GDCLASS(PKAttributeSampler, Resource);
	friend class PKAttributeList;

public:
	virtual const PParticleSamplerDescriptor &get_sampler() { return desc; }
	virtual void set_emitter(PKEmitter3D *p_emitter) { parent = p_emitter; }

protected:
	static void _bind_methods() {}
	virtual void _physics_process() {}
	virtual void _ready() {}

	PParticleSamplerDescriptor desc;
	PKEmitter3D *parent = nullptr;
};

} // namespace godot
