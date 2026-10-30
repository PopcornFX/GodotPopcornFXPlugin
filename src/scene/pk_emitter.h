//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "pk_register_types.h"

#include "integration/internal/pk_scene.h"
#include "integration/pk_sdk.h"

#include <pk_particles/include/ps_effect.h>

namespace godot {

class PKAttributeList;
class PKEffect;

#define PK_EMITTER_FORWARD_VIRTUALS(base) \
public: \
    void _ready() override { PKEmitter<base>::_ready(); } \
    void _process(double d) override { PKEmitter<base>::_process(d); } \
    void _physics_process(double d) override { PKEmitter<base>::_physics_process(d); } \
    PackedStringArray _get_configuration_warnings() const override { return PKEmitter<base>::_get_configuration_warnings(); } \
private:

template <typename Super>
class PKEmitter : public Super {
	static_assert(std::is_base_of_v<Node, Super>, "PKEmitter's Super must derive from Node");

public:
	static const StringName &get_class_static() {
		static StringName name;
		if (name.is_empty()) {
			const StringName &super_name = Super::get_class_static();
			if (super_name != StringName("Node3D") && super_name != StringName("Node2D") && super_name != StringName("Control")) {
				ERR_FAIL_V_MSG(name, vformat("PKEmitter was instantiated with unknown super %s", super_name));
			}
			name = "PKEmitter" + super_name.trim_prefix("Node");
		}
		return name;
	}

	enum TransformMode {
		TRANSFORM_DEFAULT,
		TRANSFORM_GLOBAL,
		TRANSFORM_LOCAL,
	};

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

	void set_transform_mode(TransformMode p_mode);
	TransformMode get_transform_mode() { return transform_mode; }

	PKEmitter();
	~PKEmitter();
	virtual PackedStringArray _get_configuration_warnings() const override;

protected:
	static void _bind_methods();
	void _notification(int32_t p_what);
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;

	friend class PKAttributeList;

	static inline TransformMode default_transform_mode = TRANSFORM_DEFAULT;
	TransformMode transform_mode = TRANSFORM_DEFAULT;
	CFloat4x4 effect_transform;
	CFloat4x4 effect_prev_transform;
	CFloat3 effect_velocity;
	CFloat3 effect_prev_velocity;

	friend class PKManager;

	static const int PKNOTIFICATION_ENTER_WORLD = 96000;
	static const int PKNOTIFICATION_VISIBILITY_CHANGED = 96001;

private:
	Ref<PKAttributeList> attribute_list;
	Ref<PKEffect> effect;
	PParticleEffectInstance effect_instance;
	bool is_playing;
	bool is_disabled;

	int _convert_notification(int p_what);

	void _effect_changed();
	void _attribute_list_changed();

	void _reset_transforms();
	virtual void _update_transforms() = 0;
	virtual PKScene *_get_pk_scene() = 0;

	// TEMP: make this templated class compile on unix compilers. Ideally we'll be able to find a better fix later.
	void update_configuration_warnings() { Super::update_configuration_warnings(); }
	template <typename... Args>
	Error emit_signal(const StringName &p_signal, const Args &...p_args) { return Super::template emit_signal<Args...>(p_signal, p_args...); }
	void notify_property_list_changed() { Super::notify_property_list_changed(); }
	bool is_visible() const { return Super::is_visible(); }
	bool is_node_ready() const { return Super::is_node_ready(); }
};

} // namespace godot
