//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_emitter.h"

#include "godot_cpp/classes/node2d.hpp"

namespace godot {

class PKAttributeList;
class PKEffect;

class PKEmitter2D : public PKEmitter<Node2D> {
	GDCLASS(PKEmitter2D, Node2D)

	PK_EMITTER_FORWARD_VIRTUALS(Node2D)

private:
	virtual void _update_transforms() override;
	virtual PKScene *_get_pk_scene() override;
};

} // namespace godot
