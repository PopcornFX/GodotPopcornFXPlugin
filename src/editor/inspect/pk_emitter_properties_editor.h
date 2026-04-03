//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/check_box.hpp"
#include "godot_cpp/classes/color_picker_button.hpp"
#include "godot_cpp/classes/editor_property.hpp"
#include "godot_cpp/classes/editor_spin_slider.hpp"
#include "godot_cpp/classes/foldable_container.hpp"
#include "godot_cpp/classes/grid_container.hpp"
#include "godot_cpp/templates/hash_map.hpp"

#include "integration/pk_sdk.h"
#include "scene/pk_emitter_3d.h"

#include <pk_particles/include/ps_attributes.h>

namespace godot {

class PKEmitterPropertiesEditor : public FoldableContainer {
	GDCLASS(PKEmitterPropertiesEditor, FoldableContainer);

public:
	PKEmitterPropertiesEditor(PKEmitter3D *p_emitter);
	PKEmitterPropertiesEditor() = default;

private:
	static void _bind_methods();

	static void _set_property_on_object(StringName p_property, const Variant &p_value, StringName p_field, bool p_changing, Object *p_object);

	void _set_attribute_by_name(const Variant &p_value, const String &p_attribute_name, int p_type_id, int p_index);
	void _set_attribute_by_id(const Variant &p_value, uint32_t p_attribute_id, int p_type_id, int p_index);
	void _set_color_attribute_by_id(const Color &p_value, uint32_t p_attribute_id, bool p_use_float);

	GridContainer *grid;
	PKEmitter3D *emitter;

	HashMap<String, Pair<Control *, Control *>> categories; // name: category's foldable, category's grid

	void _populate_children();
	void _reload();

	EditorProperty *_make_property_editor(Object *p_object, const Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide = false);

	Control *_get_category(const String &p_title);
	Control *_make_sampler_container(Control *p_parent_category, const String &p_title, const SParticleDeclaration::SSampler::ESamplerType p_type);

	void _make_audio_attribute_sampler(PKEmitter3D *p_emitter, uint32_t p_sampler_index, Control *p_sampler_container);

	template <typename Inner>
	Control *_make_numerical_editor_grid(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_length, Inner p_minimum_step);
	template <typename Inner>
	Control *_make_numerical_editor(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value);
	template <typename Inner>
	Control *_make_editor_property(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value);
	template <typename Inner>
	EditorSpinSlider *_make_slider(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value);

	GridContainer *_make_checkbox_grid(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_length);
	CheckBox *_make_checkbox(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, bool p_value);

	ColorPickerButton *_make_color_picker(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, bool p_use_float, bool p_edit_alpha);
};
} //namespace godot
