//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_emitter.h"

#include "godot_cpp/classes/control.hpp"

namespace godot {

class PKAttributeList;
class PKEffect;

class PKEmitterControl : public PKEmitter<Control> {
	GDCLASS(PKEmitterControl, Control)

	PK_EMITTER_FORWARD_VIRTUALS(Control)

public:
	PKEmitterControl();

private:
	virtual void _update_transforms() override;
	virtual PKScene *_get_pk_scene() override;
};

} // namespace godot
