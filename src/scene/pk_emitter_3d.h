//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_emitter.h"

#include "godot_cpp/classes/node3d.hpp"

namespace godot {

class PKAttributeList;
class PKEffect;

class PKEmitter3D : public PKEmitter<Node3D> {
	GDCLASS(PKEmitter3D, Node3D)

	PK_EMITTER_FORWARD_VIRTUALS(Node3D)
	
private:
	virtual void _update_transforms() override;
	virtual PKScene *_get_pk_scene() override;
};

} // namespace godot
