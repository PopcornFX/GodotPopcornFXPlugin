//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/variant/string_name.hpp"

namespace godot {

class PKStringNames {
	static PKStringNames *singleton;

public:
	static const PKStringNames *get_singleton();
	static void create_singleton();
	static void destroy_singleton();

	PKStringNames();

	// These are usually in CoreStringNames, but when compiling as a GDExtension, we don't have access to them
	StringName changed;
	StringName frame_post_draw;
};

class NotificationNames : public Node3D {
	GDCLASS(NotificationNames, Node3D);

public:
	static String get_notification_name(int p_what);
};

} // namespace godot
