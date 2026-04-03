//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_emitter_properties_editor.h"

#include "godot_cpp/classes/box_container.hpp"
#include "godot_cpp/classes/check_button.hpp"
#include "godot_cpp/classes/color_picker.hpp"
#include "godot_cpp/classes/editor_inspector.hpp"
#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/editor_resource_picker.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/classes/margin_container.hpp"
#include "godot_cpp/classes/option_button.hpp"
#include "godot_cpp/classes/spin_box.hpp"
#include "godot_cpp/classes/style_box_flat.hpp"

#include "core/pk_attribute_list.h"
#include "core/samplers/pk_attribute_sampler_audio.h"
#include "core/samplers/pk_attribute_sampler_image.h"
#include "core/samplers/pk_attribute_sampler_shape.h"
#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"

// not sure where to put this callback. Should this be a method ?
// this is used for the audio attribute sampler, so the field changes when we select the right thing
// this could be useful for other samplers, when a dropdown should hide every field and show only one of them
static void show_single_children(int32_t p_idx_of_child_to_show, Control *p_parent) {
	const int32_t count = p_parent->get_child_count();
	for (int32_t i = 0; i < count; ++i) {
		Control *node = reinterpret_cast<Control *>(p_parent->get_child(i)); // Reasonably assumes our ui only has control children.
		if (i == p_idx_of_child_to_show) {
			node->show();
		} else {
			node->hide();
		}
	}
}

static void update_audio_sampler_bus_enums(OptionButton *p_bus_property, OptionButton *p_effect_index_property, const Ref<PKAttributeSamplerAudio> p_sampler) {
	const AudioServer *audio_server = AudioServer::get_singleton();

	const String selected_bus_name = p_sampler->get_bus_name();
	const int32_t selected_bus_index = audio_server->get_bus_index(selected_bus_name);

	p_bus_property->clear();
	p_effect_index_property->clear();

	p_bus_property->add_item("(None)"); // Option for if we dont want to get data from the engine in this sampler.

	p_bus_property->select(-1);
	for (uint32_t i = 0; i < audio_server->get_bus_count(); ++i) {
		const String name = audio_server->get_bus_name(i);
		p_bus_property->add_item(name);
		if (selected_bus_name == name) {
			// Reselect the previously selected bus.
			p_bus_property->select(i + 1); // +1 because we have the empty element
		}
	}

	if (p_bus_property->get_selected() <= 0) { // if it's undefined (-1) or empty (0)
		p_bus_property->select(0);
	} else {
		p_effect_index_property->select(-1);
		const uint32_t effects_count = AudioServer::get_singleton()->get_bus_effect_count(selected_bus_index);

		uint32_t count = 0;
		for (uint32_t i = 0; i < effects_count; ++i) {
			Ref<AudioEffect> effect = AudioServer::get_singleton()->get_bus_effect(selected_bus_index, i);
			if (effect->is_class(PKAudioEffectCapture::get_class_static())) {
				String name = "PKCapture at index " + String::num_int64(i);
				p_effect_index_property->add_item(name, i);
				++count;
			}
		}
	}
}

static void audio_bus_updated_callback(int32_t index, OptionButton *p_bus_property, OptionButton *p_effect_index_property, Ref<PKAttributeSamplerAudio> p_sampler) {
	if (index <= 0) { // None selected
		p_sampler->set_bus_name("");
	} else {
		const String name = p_bus_property->get_item_text(index);
		p_sampler->set_bus_name(name);
	}
	update_audio_sampler_bus_enums(p_bus_property, p_effect_index_property, p_sampler);
}

static void audio_effect_idx_updated_callback(int32_t index, OptionButton *p_bus_property, OptionButton *p_effect_index_property, const Ref<PKAttributeSamplerAudio> p_sampler) {
	if (index >= 0) {
		p_sampler->set_effect_index(p_effect_index_property->get_item_id(index));
	}
}

namespace godot {
PKEmitterPropertiesEditor::PKEmitterPropertiesEditor(PKEmitter3D *p_emitter) :
		emitter(p_emitter) {
	_populate_children();
	p_emitter->connect("effect_changed", callable_mp(this, &PKEmitterPropertiesEditor::_reload));
	const Ref<PKAttributeList> attribute_list = emitter->get_attribute_list();
	if (attribute_list.is_valid()) {
		attribute_list->connect("changed", callable_mp(this, &PKEmitterPropertiesEditor::_reload));
	}
}

void PKEmitterPropertiesEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_attribute_by_id", "p_value", "p_attribute_id", "typeid", "p_index"), &PKEmitterPropertiesEditor::_set_attribute_by_id, DEFVAL(Variant()), DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_attribute_by_name", "p_value", "attribute_name", "typeid", "p_index"), &PKEmitterPropertiesEditor::_set_attribute_by_name, DEFVAL(Variant()), DEFVAL(String()), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_color_attribute_by_id", "color", "p_attribute_id", "use_float"), &PKEmitterPropertiesEditor::_set_color_attribute_by_id, DEFVAL(Variant()), DEFVAL(0), DEFVAL(false));
}

void PKEmitterPropertiesEditor::_set_property_on_object(StringName p_property, const Variant &p_value, StringName p_field, bool p_changing, Object *p_object) {
	if (p_object != nullptr) {
		p_object->set(p_property, p_value);
	}
}

void PKEmitterPropertiesEditor::_set_attribute_by_name(const Variant &p_value, const String &p_attribute_name, int p_type_id, int p_index) {
	const uint32_t id = emitter->get_attribute_list()->get_attribute_desc(p_attribute_name.utf8().get_data())->get_index();
	_set_attribute_by_id(p_value, id, p_type_id, p_index);
}

void PKEmitterPropertiesEditor::_set_attribute_by_id(const Variant &p_value, uint32_t p_attribute_id, int p_type_id, int p_index) {
	const EBaseTypeID type_id = EBaseTypeID(p_type_id);
	SAttributesContainer_SAttrib data{};
	emitter->get_attribute_list()->get_attribute(p_attribute_id, data);

	switch (type_id) {
		case BaseType_Float:
		case BaseType_Float2:
		case BaseType_Float3:
		case BaseType_Float4: {
			data.m_Data32f[p_index] = float(p_value);
			emitter->get_attribute_list()->set_attribute(p_attribute_id, data);
			break;
		}
		case BaseType_I32:
		case BaseType_Int2:
		case BaseType_Int3:
		case BaseType_Int4: {
			data.m_Data32i[p_index] = int(p_value);
			emitter->get_attribute_list()->set_attribute(p_attribute_id, data);
			break;
		}
		case BaseType_Bool:
		case BaseType_Bool2:
		case BaseType_Bool3:
		case BaseType_Bool4: {
			data.m_Data8b[p_index] = bool(p_value);
			emitter->get_attribute_list()->set_attribute(p_attribute_id, data);
			break;
		}
		default:
			PK_ASSERT_NOT_REACHED(); // Type not supported, fix ASAP if this is hit.
			break;
	}
}

void PKEmitterPropertiesEditor::_set_color_attribute_by_id(const Color &p_value, uint32_t p_attribute_id, bool p_use_float) {
	const Color corrected = p_value.srgb_to_linear();
	if (p_use_float) {
		emitter->get_attribute_list()->set_attribute(p_attribute_id, *reinterpret_cast<const SAttributesContainer_SAttrib *>(&corrected));
	} else {
		SAttributesContainer_SAttrib data;
		data.m_Data32u[0] = corrected.get_r8();
		data.m_Data32u[1] = corrected.get_g8();
		data.m_Data32u[2] = corrected.get_b8();
		data.m_Data32u[3] = corrected.get_a8();
		emitter->get_attribute_list()->set_attribute(p_attribute_id, data);
	}
}

void PKEmitterPropertiesEditor::_populate_children() {
	if (!emitter->get_effect().is_valid()) {
		set_visible(false);
		return;
	}
	const TMemoryView<CParticleAttributeDeclaration *const> declarations = emitter->get_attribute_list()->all_attribute_declarations();
	const TMemoryView<CParticleAttributeSamplerDeclaration *const> sampler_declarations = emitter->get_attribute_list()->all_attribute_sampler_declarations();

	if (declarations.Count() == 0 && sampler_declarations.Count() == 0) {
		set_visible(false);
		return;
	}

	set_visible(true);

	grid = memnew(GridContainer);
	grid->set_columns(1);
	add_child(grid);

	set_title("Attributes");
	set_h_size_flags(SIZE_EXPAND_FILL);
	set_focus_mode(FocusMode::FOCUS_ACCESSIBILITY);

	Control *category = nullptr;

	for (uint32_t i = 0; i < declarations.Count(); i++) {
		const CParticleAttributeDeclaration *declaration = declarations[i];
		category = _get_category(declaration->CategoryName().MapENG().ToUTF8().Data());

		HBoxContainer *container = memnew(HBoxContainer);

		category->add_child(container);

		Label *label = memnew(Label());
		label->set_text(declaration->ExportedName().Data());
		container->add_child(label, false);
		switch (declaration->ExportedType()) {
			case PopcornFX::BaseType_Bool:
				container->add_child(_make_checkbox_grid(declaration, i, 1));
				break;
			case PopcornFX::BaseType_Bool2:
				container->add_child(_make_checkbox_grid(declaration, i, 2));
				break;
			case PopcornFX::BaseType_Bool3:
				container->add_child(_make_checkbox_grid(declaration, i, 3));
				break;
			case PopcornFX::BaseType_Bool4:
				container->add_child(_make_checkbox_grid(declaration, i, 4));
				break;
			case PopcornFX::BaseType_I32:
				container->add_child(_make_numerical_editor_grid<int>(declaration, i, 1, 1));
				break;
			case PopcornFX::BaseType_Int2:
				container->add_child(_make_numerical_editor_grid<int>(declaration, i, 2, 1));
				break;
			case PopcornFX::BaseType_Int3:
				if (declaration->GetEffectiveDataSemantic() == DataSemantic_Color) {
					container->add_child(_make_color_picker(declaration, i, false, false));
				} else {
					container->add_child(_make_numerical_editor_grid<int>(declaration, i, 3, 1));
				}
				break;
			case PopcornFX::BaseType_Int4:
				if (declaration->GetEffectiveDataSemantic() == DataSemantic_Color) {
					container->add_child(_make_color_picker(declaration, i, false, true));
				} else {
					container->add_child(_make_numerical_editor_grid<int>(declaration, i, 4, 1));
				}
				break;
			case PopcornFX::BaseType_Float:
				container->add_child(_make_numerical_editor_grid<float>(declaration, i, 1, 0.01));
				break;
			case PopcornFX::BaseType_Float2:
				container->add_child(_make_numerical_editor_grid<float>(declaration, i, 2, 0.01));
				break;
			case PopcornFX::BaseType_Float3:
				if (declaration->GetEffectiveDataSemantic() == DataSemantic_Color) {
					container->add_child(_make_color_picker(declaration, i, true, false));
				} else {
					container->add_child(_make_numerical_editor_grid<float>(declaration, i, 3, 0.01));
				}
				break;
			case PopcornFX::BaseType_Float4:
				if (declaration->GetEffectiveDataSemantic() == DataSemantic_Color) {
					container->add_child(_make_color_picker(declaration, i, true, true));
				} else {
					container->add_child(_make_numerical_editor_grid<float>(declaration, i, 4, 0.01));
				}
				break;
			default:
				Label *new_label = memnew(Label());
				new_label->set_text("Unsupported");
				container->add_child(new_label);
		}
	}

	for (uint32_t i = 0; i < sampler_declarations.Count(); i++) {
		const CParticleAttributeSamplerDeclaration *declaration = sampler_declarations[i];
		category = _get_category(declaration->CategoryName().MapENG().ToUTF8().Data());

		// for now we get under the category's foldable instead of its grid container
		Control *sampler_container = _make_sampler_container(category, declaration->ExportedName().Data(), SParticleDeclaration::SSampler::ESamplerType(declaration->ExportedType()));

		// TEMP ! So the other samplers on't break directly. Make the other samplers use sampler_container asap !!
		category = sampler_container;

		switch (declaration->ExportedType()) {
			case SParticleDeclaration::SSampler::Sampler_Geometry: {
				Ref<PKAttributeSamplerShape> sampler = emitter->get_attribute_list()->get_attribute_sampler_or_default<PKAttributeSamplerShape>(i);
				VBoxContainer *vbox = memnew(VBoxContainer);
				vbox->set_h_size_flags(SIZE_EXPAND_FILL);

				HBoxContainer *hbox1 = memnew(HBoxContainer);
				vbox->add_child(hbox1);

				Label *label = memnew(Label());
				label->set_text("Sample Mode");
				hbox1->add_child(label, false);

				EditorProperty *property = _make_property_editor(*sampler, Variant::INT, "sample_mode", PROPERTY_HINT_ENUM, "Shape,Node", PROPERTY_USAGE_STORAGE);
				property->set_h_size_flags(SIZE_EXPAND_FILL);
				hbox1->add_child(property);

				HBoxContainer *hbox2 = memnew(HBoxContainer);
				vbox->add_child(hbox2);

				label = memnew(Label());
				label->set_text("Transform Mode");
				hbox2->add_child(label, false);

				property = _make_property_editor(*sampler, Variant::INT, "transform_mode", PROPERTY_HINT_ENUM, "None,Parent,Node", PROPERTY_USAGE_STORAGE);
				property->set_h_size_flags(SIZE_EXPAND_FILL);
				hbox2->add_child(property);

				HBoxContainer *hbox3 = memnew(HBoxContainer);
				vbox->add_child(hbox3);

				label = memnew(Label());
				hbox3->add_child(label, false);

				if (sampler->get_sample_mode() == PKAttributeSamplerShape::SAMPLE_SHAPE) {
					label->set_text("Shape");
					property = _make_property_editor(*sampler, Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "BoxShape3D,CapsuleShape3D,CylinderShape3D,SphereShape3D,ConvexPolygonShape3D,ConcavePolygonShape3D,Mesh,MeshInstance3D", PROPERTY_USAGE_STORAGE);
					property->set_h_size_flags(SIZE_EXPAND_FILL);
					hbox3->add_child(property);
				} else if (sampler->get_sample_mode() == PKAttributeSamplerShape::SAMPLE_NODE) {
					label->set_text("Node");
					property = _make_property_editor(*sampler, Variant::NODE_PATH, "node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "MeshInstance3D", PROPERTY_USAGE_STORAGE);
					property->set_h_size_flags(SIZE_EXPAND_FILL);
					hbox3->add_child(property);
				}

				category->add_child(vbox);
				break;
			}
			case SParticleDeclaration::SSampler::Sampler_Audio: {
				_make_audio_attribute_sampler(emitter, i, sampler_container);
				break;
			}
			case SParticleDeclaration::SSampler::Sampler_Image: {
				Ref<PKAttributeSamplerImage> sampler = *emitter->get_attribute_list()->get_attribute_sampler_or_default<PKAttributeSamplerImage>(i);
				EditorProperty *property = _make_property_editor(*sampler, Variant::OBJECT, "image_resource", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D", PROPERTY_USAGE_STORAGE);
				property->set_h_size_flags(SIZE_EXPAND_FILL);
				category->add_child(property);
				break;
			}
			default: {
				Label *new_label = memnew(Label());
				new_label->set_text("Unsupported");
				category->add_child(new_label);
				break;
			}
		}
	}

	for (const KeyValue<String, Pair<Control *, Control *>> &value : categories) {
		grid->add_child(value.value.first);
	}
}

void PKEmitterPropertiesEditor::_reload() {
	for (int i = 0; i < get_child_count(); i++) {
		Node *child = get_child(i);
		child->queue_free();
	}
	categories.clear();
	_populate_children();
}

EditorProperty *PKEmitterPropertiesEditor::_make_property_editor(Object *p_object, const Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, const uint32_t p_usage, const bool p_wide) {
	EditorProperty *property = EditorInspector::instantiate_property_editor(p_object, p_type, p_path, p_hint, p_hint_text, p_usage, p_wide);
	property->set_object_and_property(p_object, p_path);
	property->update_property();
	// I feel like this can't be the intended way to use EditorProperty... but I didn't find a more correct way.
	const Callable call = callable_mp_static(&PKEmitterPropertiesEditor::_set_property_on_object).bind(p_object);
	property->connect("property_changed", call);
	return property;
}

Control *PKEmitterPropertiesEditor::_get_category(const String &p_title) {
	const Pair<Control *, Control *> *category = categories.getptr(p_title);
	if (category != nullptr) {
		return category->second;
	}

	String title = p_title;
	if (title.is_empty()) {
		title = "General";
	}
	FoldableContainer *foldable = memnew(FoldableContainer);
	foldable->set_title(title);
	foldable->set_h_size_flags(SIZE_EXPAND_FILL);
	foldable->set_focus_mode(FocusMode::FOCUS_ACCESSIBILITY);

	VBoxContainer *foldable_container = memnew(VBoxContainer);
	foldable->add_child(foldable_container);

	categories[p_title] = Pair<Control *, Control *>(foldable, foldable_container);

	return foldable_container;
}

Control *PKEmitterPropertiesEditor::_make_sampler_container(Control *p_parent_category, const String &p_title, const SParticleDeclaration::SSampler::ESamplerType p_type) {
	String title = p_title;
	if (title.is_empty()) {
		title = "Unnamed Sampler";
	}

	constexpr int margin_left = 2;

	MarginContainer *margin = memnew(MarginContainer);
	margin->add_theme_constant_override("margin_left", margin_left);
	margin->set_h_size_flags(SIZE_EXPAND_FILL);

	FoldableContainer *foldable = memnew(FoldableContainer);
	foldable->set_title(title);
	foldable->set_h_size_flags(SIZE_EXPAND_FILL);
	foldable->set_focus_mode(FocusMode::FOCUS_ACCESSIBILITY);

#if 0 // Coloring depending on sampler type. It may not be a useful and godot-y feature
	static const Color color_by_sampler_type[/*number of sampler types*/] = { Color(0.41f, 0.32f, 0.63f) }; // TODO needs colors for each ofc
	Ref<StyleBoxFlat> stylebox = memnew(StyleBoxFlat());
	stylebox->set_bg_color(color_by_sampler_type[p_type]);

	foldable->add_theme_stylebox_override("title_panel", stylebox);
	foldable->add_theme_stylebox_override("title_hover_panel", stylebox);
	foldable->add_theme_stylebox_override("title_collapsed_panel", stylebox);
	foldable->add_theme_stylebox_override("title_collapsed_hover_panel", stylebox);
#endif

	VBoxContainer *foldable_container = memnew(VBoxContainer);

	foldable->add_child(foldable_container);
	margin->add_child(foldable);
	p_parent_category->add_child(margin);

	return foldable_container;
}

// Here so it does not obfuscate PopulateChildren().
void PKEmitterPropertiesEditor::_make_audio_attribute_sampler(PKEmitter3D *p_emitter, uint32_t p_sampler_index, Control *p_sampler_container) {
	PKAttributeSamplerAudio *sampler = *p_emitter->get_attribute_list()->get_attribute_sampler_or_default<PKAttributeSamplerAudio>(p_sampler_index);

	// Whether the popcorn channel is global.

	EditorProperty *is_pk_channel_global = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::BOOL, "is_pk_channel_global", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	is_pk_channel_global->set_object_and_property(sampler, "is_pk_channel_global");
	is_pk_channel_global->set_label("Is Popcorn Channel Global");
	is_pk_channel_global->set_tooltip_text("If `true`, the popcorn channel name that you set will be global.\n"
										   "Ie. all the audio samplers sharing that name share the same data.\n"
										   "If `false`, the audio captured by this sampler will affect this sampler only.\n");
	is_pk_channel_global->set_selectable(false);
	is_pk_channel_global->update_property();

	p_sampler_container->add_child(is_pk_channel_global);

	// Name of the popcorn channel.

	EditorProperty *target_pk_channel_name = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::STRING_NAME, "target_popcorn_channel_group_string", PROPERTY_HINT_TYPE_STRING, "", PROPERTY_USAGE_STORAGE);
	target_pk_channel_name->set_object_and_property(sampler, "target_popcorn_channel_group_string");
	target_pk_channel_name->set_label("Target Popcorn Channel");
	target_pk_channel_name->set_tooltip_text("The name of the channel in Popcorn to which to send the audio data, and read the audio data from.\n");
	target_pk_channel_name->set_selectable(false);
	target_pk_channel_name->update_property();

	p_sampler_container->add_child(target_pk_channel_name);

#if 0 // TODO find:  1. if we must expose this, 2. where is it possible to change it. It's not in the desc (?)
	// Whether we must send spectrum data.

	EditorProperty *is_spectrum = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::BOOL, "is_spectrum", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	is_spectrum->set_object_and_property(sampler, "is_spectrum");
	is_spectrum->set_label("Data As Spectrum");
	is_spectrum->set_tooltip_text("If `true`, a DFT will be performed on channel's data before sending it to popcorn.\n"
			"Otherwise, the waveform is sent as-is.\n"
			"Spectrum and waveform channels are separate, even when they have the same name. Ie. You can't send both a waveform and a spectrum to the same Popcorn channel.\n");
	is_spectrum->set_selectable(false);
	is_spectrum->update_property();

	p_sampler_container->add_child(is_spectrum);
#endif

#if 0 // Stashed away for now, since we don't have audiostreamplayer mode yet.
	// Creating a dropdown to switch between sources.
	OptionButton *source_select = memnew(OptionButton);
	source_select->add_item("Bus", PKAttributeSamplerAudio::BUS_AS_SOURCE);
	source_select->add_item("AudioStreamPlayer", PKAttributeSamplerAudio::AUDIOSTREAMPLAYER_AS_SOURCE);
	source_select->select(sampler->get_source_mode());
	sampler_container->add_child(source_select);
#endif

	// Creating a node to serve as a container between options.
	MarginContainer *options_container = memnew(MarginContainer);
	options_container->set_h_size_flags(SIZE_EXPAND_FILL);
	options_container->set_v_size_flags(SIZE_EXPAND_FILL);
	p_sampler_container->add_child(options_container);

	// If we're in bus mode.

	VBoxContainer *source_bus = memnew(VBoxContainer);

	// Not very pretty, will be better once we refactor to use the inspector properties the "intended" way.

	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(2);
	grid->set_h_size_flags(SIZE_EXPAND_FILL);
	source_bus->set_h_size_flags(SIZE_EXPAND_FILL);
	Label *label_bus = memnew(Label);
	label_bus->set_h_size_flags(SIZE_EXPAND_FILL);
	label_bus->set_text("Bus Name");
	Label *label_effect = memnew(Label);
	label_effect->set_h_size_flags(SIZE_EXPAND_FILL);
	label_effect->set_text("Audio Effect Index");

	OptionButton *source_bus_name = memnew(OptionButton);
	OptionButton *source_bus_effect_idx = memnew(OptionButton);
	source_bus_name->set_h_size_flags(SIZE_EXPAND_FILL);
	source_bus_name->set_allow_reselect(true);
	source_bus_effect_idx->set_h_size_flags(SIZE_EXPAND_FILL);
	source_bus_effect_idx->set_allow_reselect(true);

	update_audio_sampler_bus_enums(source_bus_name, source_bus_effect_idx, sampler);

	options_container->add_child(source_bus);
	source_bus->add_child(grid);
	grid->add_child(label_bus);
	grid->add_child(source_bus_name);
	grid->add_child(label_effect);
	grid->add_child(source_bus_effect_idx);

	const Callable call_enum_bus = callable_mp_static(&audio_bus_updated_callback).bind(sampler).bind(source_bus_effect_idx).bind(source_bus_name);
	source_bus_name->connect("item_selected", call_enum_bus);

	const Callable call_enum_effect_idx = callable_mp_static(&audio_effect_idx_updated_callback).bind(sampler).bind(source_bus_effect_idx).bind(source_bus_name);
	source_bus_effect_idx->connect("item_selected", call_enum_effect_idx);

	// If we're in audiostreamplayer mode.
	// For now, we don't do audiostreamplayer mode.
#if 0
	EditorProperty *source_audiostreamplayer = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::OBJECT, "audiostreamplayer", PROPERTY_HINT_NODE_TYPE, "AudioStreamPlayer, AudioStreamPlayer2D, AudioStreamPlayer3D", PROPERTY_USAGE_STORAGE);
	source_audiostreamplayer->set_object_and_property(sampler, "audiostreamplayer");
	source_audiostreamplayer->set_h_size_flags(SIZE_EXPAND_FILL);
	source_audiostreamplayer->set_v_size_flags(SIZE_EXPAND_FILL);
	source_audiostreamplayer->update_property();
	options_container->add_child(source_audiostreamplayer);
	if (sampler->get_source_mode() != PKAttributeSamplerAudio::AUDIOSTREAMPLAYER_AS_SOURCE) {
		source_audiostreamplayer->hide();
	}

	// Making the dropdown actually do something.
	Callable source_select_callback_change_ui = callable_mp_static(&show_single_children).bind(options_container);
	Callable source_select_callback_set_mode = callable_mp(sampler, &PKAttributeSamplerAudio::set_source_mode);
	source_select->connect("item_selected", source_select_callback_change_ui);
	source_select->connect("item_selected", source_select_callback_set_mode);
#endif

	// Add the smoothing property.
	EditorProperty *smoothing_factor = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::FLOAT, "smoothing_factor", PROPERTY_HINT_RANGE, "0, 1", PROPERTY_USAGE_STORAGE);
	smoothing_factor->set_object_and_property(sampler, "smoothing_factor");
	smoothing_factor->set_label("Smoothing Factor");
	smoothing_factor->set_tooltip_text("Interpolate the values between this frame and the previous one.\n"
									   "0 means no smoothing, 1 means the value of the last frame (no movement).");
	smoothing_factor->set_selectable(false);
	smoothing_factor->update_property();
	p_sampler_container->add_child(smoothing_factor);

	// Add the scale property.
	EditorProperty *scale_factor = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(sampler, Variant::FLOAT, "scale_factor", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE);
	scale_factor->set_object_and_property(sampler, "scale_factor");
	scale_factor->set_label("Scale Factor");
	scale_factor->set_tooltip_text("Scale the values.\n");
	scale_factor->set_selectable(false);
	scale_factor->update_property();
	p_sampler_container->add_child(scale_factor);

	// Connect the signals.
	const Callable call = callable_mp_static(&PKEmitterPropertiesEditor::_set_property_on_object).bind(sampler);
	is_pk_channel_global->connect("property_changed", call);
	target_pk_channel_name->connect("property_changed", call);

#if 0
	source_audiostreamplayer->connect("property_changed", call);
#endif
	smoothing_factor->connect("property_changed", call);
	scale_factor->connect("property_changed", call);

	// Signals that update the enum values in the UI. This is not really clean, but it'll work until we refactor all this to align more with Godot's properties.
}
template <typename Inner>
Control *PKEmitterPropertiesEditor::_make_numerical_editor_grid(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_length, Inner p_minimum_step) {
	SAttributesContainer_SAttrib data;
	emitter->get_attribute_list()->get_attribute(p_attribute_id, data);

	GridContainer *grid = memnew(GridContainer);
	grid->set_h_size_flags(SIZE_EXPAND_FILL);
	grid->set_columns(p_length);
	for (int i = 0; i < p_length; i++) {
		Control *editor = _make_numerical_editor(p_declaration, p_attribute_id, i, p_minimum_step, data.Get<Inner>()[i]);
		grid->add_child(editor);
	}
	return grid;
}

template <typename Inner>
Control *PKEmitterPropertiesEditor::_make_numerical_editor(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value) {
	if (p_declaration->UseSlider()) {
		return _make_slider(p_declaration, p_attribute_id, p_index, p_minimum_step, p_value);
	}
	return _make_editor_property(p_declaration, p_attribute_id, p_index, p_minimum_step, p_value);
}

template <typename Inner>
Control *PKEmitterPropertiesEditor::_make_editor_property(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value) {
	SpinBox *editor = memnew(SpinBox);
	if (p_declaration->HasMin()) {
		editor->set_min(*p_declaration->GetMinValue().Get<Inner>());
	} else {
		if (TTypeTraits::IsFloat<Inner>::True) {
			editor->set_min(-1'000'000'000.0); // Due to how Godot's Range clamps values, using min/-inf doesn't work
		} else {
			editor->set_min(TNumericTraits<Inner>::Min());
		}
	}

	if (p_declaration->HasMax()) {
		editor->set_max(*p_declaration->GetMaxValue().Get<Inner>());
	} else {
		if (TTypeTraits::IsFloat<Inner>::True) {
			editor->set_max(1'000'000'000.0); // Due to how Godot's Range clamps values, using max/+inf doesn't work
		} else {
			editor->set_max(TNumericTraits<Inner>::Max());
		}
	}

	if (p_declaration->HasMin() && p_declaration->HasMax()) {
		Inner range = *p_declaration->GetMaxValue().Get<Inner>() - *p_declaration->GetMinValue().Get<Inner>();
		editor->set_step(Math::max<Inner>(range / 1000, p_minimum_step));
	} else {
		editor->set_step(p_minimum_step);
	}

	editor->set_value(p_value);
	Callable call = callable_mp(this, &PKEmitterPropertiesEditor::_set_attribute_by_id);
	call = call.bind(p_index);
	call = call.bind(p_declaration->ExportedType());
	call = call.bind(p_attribute_id);
	editor->connect("value_changed", call);
	editor->set_h_size_flags(SIZE_EXPAND_FILL);
	return editor;
}

template <typename Inner>
EditorSpinSlider *PKEmitterPropertiesEditor::_make_slider(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, Inner p_minimum_step, Inner p_value) {
	EditorSpinSlider *slider = memnew(EditorSpinSlider);
	slider->set_min(p_declaration->GetMinValue().Get<Inner>()[p_index]);
	slider->set_max(p_declaration->GetMaxValue().Get<Inner>()[p_index]);
	Inner range = p_declaration->GetMaxValue().Get<Inner>()[p_index] - p_declaration->GetMinValue().Get<Inner>()[p_index];
	slider->set_step(Math::max<Inner>(range / 1000, p_minimum_step));
	slider->set_value(p_value);
	Callable call = callable_mp(this, &PKEmitterPropertiesEditor::_set_attribute_by_id);
	call = call.bind(p_index);
	call = call.bind(p_declaration->ExportedType());
	call = call.bind(p_attribute_id);
	slider->connect("value_changed", call);
	slider->set_h_size_flags(SIZE_EXPAND_FILL);
	return slider;
}

GridContainer *PKEmitterPropertiesEditor::_make_checkbox_grid(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_length) {
	SAttributesContainer_SAttrib data;
	emitter->get_attribute_list()->get_attribute(p_attribute_id, data);

	GridContainer *grid = memnew(GridContainer);
	grid->set_h_size_flags(SIZE_EXPAND_FILL);
	grid->set_columns(p_length);
	for (int i = 0; i < p_length; i++) {
		CheckBox *checkbox = _make_checkbox(p_declaration, p_attribute_id, i, data.m_Data8b[i]);
		grid->add_child(checkbox);
	}
	return grid;
}

CheckBox *PKEmitterPropertiesEditor::_make_checkbox(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, int p_index, bool p_value) {
	CheckBox *checkbox = memnew(CheckBox);
	checkbox->set_pressed_no_signal(p_value);
	Callable call = callable_mp(this, &PKEmitterPropertiesEditor::_set_attribute_by_id);
	call = call.bind(p_index);
	call = call.bind(p_declaration->ExportedType());
	call = call.bind(p_attribute_id);
	checkbox->connect("toggled", call);
	return checkbox;
}

ColorPickerButton *PKEmitterPropertiesEditor::_make_color_picker(const CParticleAttributeDeclaration *p_declaration, int p_attribute_id, bool p_use_float, bool p_edit_alpha) {
	ColorPickerButton *picker = memnew(ColorPickerButton);
	picker->set_edit_alpha(p_edit_alpha);
	picker->set_h_size_flags(SIZE_EXPAND_FILL);

	SAttributesContainer_SAttrib data;
	emitter->get_attribute_list()->get_attribute(p_attribute_id, data);

	if (!p_edit_alpha) {
		if (p_use_float) {
			data.m_Data32f[3] = 1.0f;
		} else {
			data.m_Data32u[3] = 255;
		}
	}

	Color color;
	if (p_use_float) {
		color = *reinterpret_cast<Color *>(&data);
	} else {
		color.set_r8(data.m_Data32u[0]);
		color.set_g8(data.m_Data32u[1]);
		color.set_b8(data.m_Data32u[2]);
		color.set_a8(data.m_Data32u[3]);
	}
	picker->set_pick_color(color.linear_to_srgb());

	Callable call = callable_mp(this, &PKEmitterPropertiesEditor::_set_color_attribute_by_id);
	call = call.bind(p_use_float);
	call = call.bind(p_attribute_id);
	picker->connect("color_changed", call);

	return picker;
}
} //namespace godot
