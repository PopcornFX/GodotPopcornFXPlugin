#include "pk_editor_property_bool_array.h"

#include "godot_cpp/classes/h_box_container.hpp"

PKEditorPropertyBoolArray::PKEditorPropertyBoolArray(int p_dimension) {
	HBoxContainer *hbox = memnew(HBoxContainer);
	hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(hbox);
	for (int i = 0; i < p_dimension; i++) {
		CheckBox *checkbox = memnew(CheckBox);
		checkbox->connect("toggled", callable_mp(this, &PKEditorPropertyBoolArray::_toggled));
		checkboxes.push_back(checkbox);
		hbox->add_child(checkbox);
	}
}

void PKEditorPropertyBoolArray::_update_property() {
	const Variant &value = get_edited_object()->get(get_edited_property());
	for (int i = 0; i < checkboxes.size(); i++) {
		checkboxes[i]->set_pressed_no_signal(value.get(i));
	}
}

void PKEditorPropertyBoolArray::_set_read_only(bool p_read_only) {
	for (CheckBox *checkbox : checkboxes) {
		checkbox->set_disabled(p_read_only);
	}
}

void PKEditorPropertyBoolArray::_toggled(bool p_value) {
	TypedArray<bool> array;
	for (const CheckBox *checkbox : checkboxes) {
		array.push_back(checkbox->is_pressed());
	}
	emit_changed(get_edited_property(), array, "");
}
