//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_3d.h"

#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/defs.hpp"

#include "core/pk_attribute_list.h"
#include "core/pk_effect.h"
#include "core/pk_string_names.h"
#include "integration/pk_plugin.h"

#include <pk_particles/include/ps_attributes.h>
#include <pk_particles/include/ps_effect.h>

#define DEBUG_NOTIFICATIONS 0

namespace godot {

PKEmitter3D::TransformMode PKEmitter3D::default_transform_mode = TRANSFORM_DEFAULT;

void PKEmitter3D::_physics_process(double p_delta) {
	if (!is_playing) {
		return;
	}

	_update_transforms();
	update_gizmos();
	if (attribute_list.is_valid()) {
		attribute_list->_physics_process();
	}
}

void PKEmitter3D::_ready() {
	if (attribute_list.is_valid()) {
		attribute_list->_ready();
	}
}

const PParticleEffectInstance PKEmitter3D::get_effect_instance() const {
	return effect_instance;
}

SSpawnTransformsPack PKEmitter3D::get_transform_pack() const {
	SSpawnTransformsPack pack;
	pack.m_WorldTr_Current = &effect_transform;
	pack.m_WorldTr_Previous = &effect_prev_transform;
	pack.m_WorldVel_Current = &effect_velocity;
	pack.m_WorldVel_Previous = &effect_prev_velocity;
	return pack;
}

void PKEmitter3D::set_effect(const Ref<PKEffect> &p_effect) {
	if (effect == p_effect) {
		return;
	}

	if (effect.is_valid()) {
		effect->disconnect(PKStringNames::get_singleton()->changed, callable_mp(this, &PKEmitter3D::_effect_changed));
	}

	if (effect_instance != nullptr) {
		kill_effect();
	}
	effect = p_effect;

	if (effect.is_valid()) {
		effect->connect(PKStringNames::get_singleton()->changed, callable_mp(this, &PKEmitter3D::_effect_changed));
		_effect_changed();
	} else {
		// clear
		update_gizmos();
	}

	notify_property_list_changed();
}

Ref<PKEffect> PKEmitter3D::get_effect() const {
	return effect;
}

void PKEmitter3D::set_effect_is_playing(const bool p_is_playing) {
	is_playing = p_is_playing;
	if (is_playing) {
		start_effect();
	} else {
		kill_effect();
	}
	notify_property_list_changed();
}

bool PKEmitter3D::get_effect_is_playing() const {
	return is_playing;
}

bool PKEmitter3D::start_effect() {
	if (is_disabled || effect == nullptr) {
		return false;
	}
	_reset_transforms();
	if (effect_instance == nullptr) {
		effect_instance = effect->create_instance();
	} else if (effect_instance->Alive()) {
		return true; // Already started
	}
	SEffectStartCtl effect_start_ctl;
	effect_start_ctl.m_SpawnTransformsPack = get_transform_pack();
	const bool ok = effect_instance->Start(effect_start_ctl);
	if (!ok) {
		return false;
	}
	effect_instance->SetVisible(is_visible());
	attribute_list->reapply_attributes();
	return true;
}

void PKEmitter3D::kill_effect() {
	if (effect_instance != nullptr) {
		effect_instance->KillDeferred();
		effect_instance = nullptr;
	}
}

void PKEmitter3D::set_attribute_list(Ref<PKAttributeList> p_attribute_list) {
	this->attribute_list = p_attribute_list;
	p_attribute_list->set_emitter(this);
}

Ref<PKAttributeList> PKEmitter3D::get_attribute_list() const {
	return attribute_list;
}

void PKEmitter3D::set_transform_mode(TransformMode p_mode) {
	transform_mode = p_mode;
	_reset_transforms();
}

PKEmitter3D::PKEmitter3D() {
	is_playing = true;
	is_disabled = false;
	effect_transform = to_pk(Transform3D());
	effect_prev_transform = effect_transform;
	effect_velocity = CFloat3::ZERO;
	effect_prev_velocity = effect_velocity;
}

PKEmitter3D::~PKEmitter3D() {
}

void PKEmitter3D::_bind_methods() {
	BIND_BASIC_PROPERTY(PKEmitter3D, OBJECT, attribute_list, PROPERTY_HINT_RESOURCE_TYPE, "PKAttributeList", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE)

	ClassDB::bind_method(D_METHOD("set_effect_is_playing", "effect"), &PKEmitter3D::set_effect_is_playing);
	ClassDB::bind_method(D_METHOD("get_effect_is_playing"), &PKEmitter3D::get_effect_is_playing);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_playing", PROPERTY_HINT_NONE, ""), "set_effect_is_playing", "get_effect_is_playing");

	ClassDB::bind_method(D_METHOD("set_effect", "effect"), &PKEmitter3D::set_effect);
	ClassDB::bind_method(D_METHOD("get_effect"), &PKEmitter3D::get_effect);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "PKEffect"), "set_effect", "get_effect");

	BIND_BASIC_PROPERTY(PKEmitter3D, INT, transform_mode, PROPERTY_HINT_ENUM, "Default,Global,Local", PROPERTY_USAGE_DEFAULT)

	ADD_SIGNAL(MethodInfo("effect_changed"));
	ClassDB::bind_method(D_METHOD("_effect_changed"), &PKEmitter3D::_effect_changed);

	BIND_ENUM_CONSTANT(TRANSFORM_DEFAULT);
	BIND_ENUM_CONSTANT(TRANSFORM_GLOBAL);
	BIND_ENUM_CONSTANT(TRANSFORM_LOCAL);
}

void PKEmitter3D::_notification(int32_t p_what) {
	switch (p_what) {
		case NOTIFICATION_PREDELETE:
			if (effect_instance != nullptr) {
				kill_effect();
			}
			break;
		case NOTIFICATION_ENTER_TREE:
			is_disabled = false;
			if (is_node_ready() && is_playing) {
				start_effect();
			}
			break;
#if DEBUG_NOTIFICATIONS
			UtilityFunctions::print(NotificationNames::get_notification_name(p_what), " ", (size_t)this);
#endif
			break;
		case NOTIFICATION_EXIT_TREE:
			is_disabled = true;
			if (is_node_ready() && is_playing && effect_instance != nullptr) {
				kill_effect();
			}
			break;
#if DEBUG_NOTIFICATIONS
			UtilityFunctions::print(NotificationNames::get_notification_name(p_what), " ", (size_t)this);
#endif
			break;
		case NOTIFICATION_ENTER_WORLD: {
			is_disabled = false;
			if (effect_instance != nullptr && is_playing && !is_disabled) {
				if (!start_effect()) {
					CLog::Log(PK_WARN, "PKEmitter3D Failed to start effect");
				}
			}
			break;
		}
		case NOTIFICATION_VISIBILITY_CHANGED:
			if (effect_instance != nullptr) {
				effect_instance->SetVisible(is_visible());
			}
			break;
#if DEBUG_NOTIFICATIONS
		case NOTIFICATION_PROCESS:
		case NOTIFICATION_WM_MOUSE_ENTER:
		case NOTIFICATION_WM_MOUSE_EXIT:
		case NOTIFICATION_WM_WINDOW_FOCUS_IN:
		case NOTIFICATION_WM_WINDOW_FOCUS_OUT:
		case NOTIFICATION_APPLICATION_FOCUS_IN:
		case NOTIFICATION_APPLICATION_FOCUS_OUT:
			break;
		default:
			UtilityFunctions::print(NotificationNames::get_notification_name(p_what), " ", (size_t)this);
#endif // DEBUG_NOTIFICATIONS
	}
}

void PKEmitter3D::_effect_changed() {
	ERR_FAIL_COND(effect.is_null());

	if (effect_instance != nullptr) {
		kill_effect();
	}

	effect_instance = effect->create_instance();

	if (effect->get_effect() == nullptr) {
		return;
	}

	if (attribute_list.is_null() || attribute_list->get_effect().is_null() || attribute_list->get_effect()->get_load_path() != effect->get_load_path()) {
		attribute_list = PKAttributeList::default_for_emitter(this);
	} else {
		attribute_list->set_effect(effect);
		attribute_list->resolve_effect_change();
	}

	set_effect_is_playing(is_playing);

	update_gizmos();
	emit_signal("effect_changed");
}

void PKEmitter3D::_update_transforms() {
	if (!is_inside_tree()) {
		return;
	}
	effect_prev_transform = effect_transform;
	if ((transform_mode == TRANSFORM_DEFAULT && default_transform_mode == TRANSFORM_LOCAL) || transform_mode == TRANSFORM_LOCAL) {
		effect_transform = to_pk(get_transform());
	} else {
		effect_transform = to_pk(get_global_transform());
	}
	effect_prev_velocity = effect_velocity;
	effect_velocity = effect_transform.Translations().xyz() - effect_prev_transform.Translations().xyz();
}

void PKEmitter3D::_reset_transforms() {
	_update_transforms();
	effect_prev_transform = effect_transform;
	effect_prev_velocity = effect_velocity;
}

} // namespace godot

VARIANT_ENUM_CAST(PKEmitter3D::TransformMode);
