//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_3d.h"

#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_settings.hpp"
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

Variant::Type pk_type_to_gd_type(EBaseTypeID p_pk_type, EDataSemantic p_semantic, PropertyHint &r_hint_type, String &r_hint) {
	switch (p_pk_type) {
		case BaseType_I32:
			return Variant::INT;
		case BaseType_Int2:
			return Variant::VECTOR2I;
		case BaseType_Int3:
			return Variant::VECTOR3I;
		case BaseType_Int4:
			return Variant::VECTOR4I;

		case BaseType_Float:
			return Variant::FLOAT;
		case BaseType_Float2:
			return Variant::VECTOR2;
		case BaseType_Float3:
			if (p_semantic == DataSemantic_Color) {
				r_hint_type = PROPERTY_HINT_COLOR_NO_ALPHA;
				return Variant::COLOR;
			}
			return Variant::VECTOR3;
		case BaseType_Float4:
			if (p_semantic == DataSemantic_Color) {
				return Variant::COLOR;
			}
			return Variant::VECTOR4;

		case BaseType_Bool:
			return Variant::BOOL;
		case BaseType_Bool2:
		case BaseType_Bool3:
		case BaseType_Bool4:
			r_hint = "PKAttributeBool" + String::num_int64(CBaseTypeTraits::Traits(p_pk_type).VectorDimension);
			return Variant::ARRAY;

		default:
			return Variant::NIL;
	}
}

template <typename T>
T get_min_value(T *p_values, int p_size) {
	T min = p_values[0];
	for (int i = 0; i < p_size; i++) {
		min = MIN(min, p_values[i]);
	}
	return min;
}

template <typename T>
T get_max_value(T *p_values, int p_size) {
	T max = p_values[0];
	for (int i = 0; i < p_size; i++) {
		max = MAX(max, p_values[i]);
	}
	return max;
}

String make_range_hint(const CParticleAttributeDeclaration *p_decl) {
	if (!p_decl->HasMin() && !p_decl->HasMax() && !p_decl->UseSlider()) {
		return "";
	}
	const CBaseTypeTraits &traits = CBaseTypeTraits::Traits(p_decl->GetType());
	double min = 0;
	double max = 0;
	double step = 0;

	if (p_decl->HasMin()) {
		if (traits.IsFp) {
			min = get_min_value(p_decl->GetMinValue().Get<float>(), traits.VectorDimension);
		} else {
			min = get_min_value(p_decl->GetMinValue().Get<PopcornFX::s32>(), traits.VectorDimension);
		}
	} else {
		if (traits.IsFp) {
			min = -1'000'000'000.0; // Due to how Godot's Range clamps values, using min/-inf doesn't work
		} else {
			min = TNumericTraits<PopcornFX::s32>::Min();
		}
	}

	if (p_decl->HasMax()) {
		if (traits.IsFp) {
			max = get_max_value(p_decl->GetMaxValue().Get<float>(), traits.VectorDimension);
		} else {
			max = get_max_value(p_decl->GetMaxValue().Get<PopcornFX::s32>(), traits.VectorDimension);
		}
	} else {
		if (traits.IsFp) {
			max = 1'000'000'000.0; // Due to how Godot's Range clamps values, using min/-inf doesn't work
		} else {
			max = TNumericTraits<PopcornFX::s32>::Max();
		}
	}

	double minimum_step = 0.01;
	if (!traits.IsFp) {
		minimum_step = 1;
	}

	if (p_decl->HasMin() && p_decl->HasMax()) {
		step = MAX((max - min) / 1000, minimum_step);
	} else {
		step = MAX(0.1, minimum_step);
	}

	String hint = vformat("%.2f,%.2f,%.2f", min, max, step);
	if (p_decl->UseSlider()) {
		hint = hint + ",prefer_slider";
	}

	return hint;
}

namespace godot {

PKEmitter3D::TransformMode PKEmitter3D::default_transform_mode = TRANSFORM_DEFAULT;

void PKEmitter3D::_physics_process(double p_delta) {
	if (!is_playing) {
		return;
	}

	_update_transforms();
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
		notify_property_list_changed();
		update_configuration_warnings();
	}
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
	ERR_FAIL_NULL(p_attribute_list);
	attribute_list = p_attribute_list;
	Callable call = callable_mp(this, &PKEmitter3D::_attribute_list_changed);
	if (!attribute_list->is_connected("changed", call)) {
		attribute_list->connect("changed", call);
	}
	attribute_list->set_emitter(this);
	notify_property_list_changed();
	update_configuration_warnings();
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
	is_disabled = true;
	effect_transform = to_pk(Transform3D());
	effect_prev_transform = effect_transform;
	effect_velocity = CFloat3::ZERO;
	effect_prev_velocity = effect_velocity;
}

PKEmitter3D::~PKEmitter3D() {
}

PackedStringArray PKEmitter3D::_get_configuration_warnings() const {
	if (attribute_list.is_valid()) {
		return attribute_list->_get_configuration_warnings();
	}
	return {};
}

void PKEmitter3D::_bind_methods() {
	BIND_BASIC_PROPERTY(PKEmitter3D, OBJECT, attribute_list, PROPERTY_HINT_RESOURCE_TYPE, "PKAttributeList", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);

	ClassDB::bind_method(D_METHOD("set_effect_is_playing", "effect"), &PKEmitter3D::set_effect_is_playing);
	ClassDB::bind_method(D_METHOD("get_effect_is_playing"), &PKEmitter3D::get_effect_is_playing);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_playing", PROPERTY_HINT_NONE, ""), "set_effect_is_playing", "get_effect_is_playing");

	ClassDB::bind_method(D_METHOD("set_effect", "effect"), &PKEmitter3D::set_effect);
	ClassDB::bind_method(D_METHOD("get_effect"), &PKEmitter3D::get_effect);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "effect", PROPERTY_HINT_RESOURCE_TYPE, "PKEffect"), "set_effect", "get_effect");

	BIND_BASIC_PUBLIC_PROPERTY(PKEmitter3D, INT, transform_mode, PROPERTY_HINT_ENUM, "Default,Global,Local", PROPERTY_USAGE_DEFAULT);

	ADD_SIGNAL(MethodInfo("effect_changed"));
	ClassDB::bind_method(D_METHOD("_effect_changed"), &PKEmitter3D::_effect_changed);

	BIND_ENUM_CONSTANT(TRANSFORM_DEFAULT);
	BIND_ENUM_CONSTANT(TRANSFORM_GLOBAL);
	BIND_ENUM_CONSTANT(TRANSFORM_LOCAL);
}

void PKEmitter3D::_notification(int32_t p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			Node *node = memnew(Node);
			add_child(node);
			node->set_owner(this);
			break;
		}
		case NOTIFICATION_PREDELETE:
			if (effect_instance != nullptr) {
				kill_effect();
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
			if (effect.is_valid() && is_playing) {
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

void PKEmitter3D::_get_property_list(List<PropertyInfo> *p_list) const {
	if (attribute_list.is_null() || attribute_list->all_declarations().Empty()) {
		return;
	}
	PropertyUsageFlags parent_usage = PROPERTY_USAGE_CATEGORY;
	PropertyUsageFlags child_usage = PROPERTY_USAGE_GROUP;
	if (EditorInterface::get_singleton()->get_editor_settings()->get_setting("popcornfx/emitters/foldable_attributes")) {
		parent_usage = PROPERTY_USAGE_GROUP;
		child_usage = PROPERTY_USAGE_SUBGROUP;
	}
	p_list->push_back(PropertyInfo(
			Variant::NIL,
			"Emitter Attributes",
			PROPERTY_HINT_NONE, "",
			parent_usage));

	String current_category = "";
	p_list->push_back(PropertyInfo(
			Variant::NIL,
			"General",
			PROPERTY_HINT_NONE, "",
			child_usage));
	CParticleAttributeList::_TypeOfAttributeAndSamplerList all_decl = attribute_list->all_declarations();
	for (int i = 0; i < all_decl.Count(); i++) {
		CParticleAttributeDeclarationAbstract *decl = all_decl[i];
		const String category = to_gd(decl->CategoryName());
		if (category != "" && category != current_category) {
			current_category = category;
			p_list->push_back(PropertyInfo(
					Variant::NIL,
					current_category,
					PROPERTY_HINT_NONE, "",
					child_usage));
		}

		if (!decl->IsSampler()) {
			CParticleAttributeDeclaration *attr_decl = static_cast<CParticleAttributeDeclaration *>(decl);
			PropertyHint hint_type = PROPERTY_HINT_NONE;
			String hint = "";
			const Variant::Type type = pk_type_to_gd_type(attr_decl->GetType(), attr_decl->GetEffectiveDataSemantic(), hint_type, hint);

			if (hint_type == PROPERTY_HINT_NONE && hint == "") {
				hint = make_range_hint(attr_decl);
				if (hint != "") {
					hint_type = PROPERTY_HINT_RANGE;
				}
			}
			p_list->push_back(PropertyInfo(
					type,
					to_gd(attr_decl->ExportedName()),
					hint_type, hint,
					PROPERTY_USAGE_EDITOR));
		} else {
			const CParticleAttributeSamplerDeclaration *sampler_decl = static_cast<CParticleAttributeSamplerDeclaration *>(decl);
			Ref<PKAttributeSamplerDesc> desc = attribute_list->get_attribute_sampler_desc(to_gd(sampler_decl->ExportedName()));
			if (desc.is_null()) {
				continue;
			}
			const Ref<PKAttributeSampler> sampler = attribute_list->get_attribute_sampler(desc->get_index());
			if (sampler.is_valid()) {
				p_list->push_back(PropertyInfo(
						Variant::OBJECT,
						to_gd(sampler_decl->ExportedName()),
						PROPERTY_HINT_RESOURCE_TYPE, sampler->get_class(),
						PROPERTY_USAGE_EDITOR));
			} else {
				p_list->push_back(PropertyInfo(
						Variant::NIL,
						to_gd(sampler_decl->ExportedName()),
						PROPERTY_HINT_NONE, "",
						PROPERTY_USAGE_EDITOR));
			}
		}
	}
}

bool PKEmitter3D::_set(const StringName &p_name, const Variant &p_property) {
	if (attribute_list.is_null() || attribute_list->effect.is_null()) {
		return false;
	}
	if (attribute_list->get_attribute_desc(p_name).is_valid()) {
		return attribute_list->set_attribute_variant(p_name, p_property);
	}
	if (attribute_list->get_attribute_sampler_desc(p_name).is_valid()) {
		return attribute_list->set_attribute_sampler(p_name, p_property);
	}
	return false;
}

bool PKEmitter3D::_get(const StringName &p_name, Variant &r_property) const {
	if (attribute_list.is_null() || attribute_list->effect.is_null()) {
		return false;
	}
	if (attribute_list->get_attribute_desc(p_name) != nullptr) {
		r_property = attribute_list->get_attribute_variant(p_name);
		return r_property.get_type() != Variant::NIL;
	}
	if (attribute_list->get_attribute_sampler_desc(p_name) != nullptr) {
		const Ref<PKAttributeSampler> sampler = attribute_list->get_attribute_sampler(p_name);
		r_property = sampler;
		const Variant::Type type = r_property.get_type();
		return type != Variant::NIL;
	}
	return false;
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
		attribute_list->_ready();
	} else {
		attribute_list->set_effect(effect);
		attribute_list->resolve_effect_change();
	}
	notify_property_list_changed();
	update_configuration_warnings();

	set_effect_is_playing(is_playing);

	emit_signal("effect_changed");
}

void PKEmitter3D::_attribute_list_changed() {
	update_configuration_warnings();
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
