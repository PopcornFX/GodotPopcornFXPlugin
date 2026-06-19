#pragma once

#include "godot_cpp/classes/check_box.hpp"
#include "godot_cpp/classes/editor_property.hpp"

using namespace godot;

class PKEditorPropertyBoolArray : public EditorProperty {
	GDCLASS(PKEditorPropertyBoolArray, EditorProperty);

public:
	PKEditorPropertyBoolArray() = default;
	PKEditorPropertyBoolArray(int p_dimension);
	virtual void _update_property() override;
	virtual void _set_read_only(bool p_read_only) override;

protected:
	static void _bind_methods() {}

private:
	Vector<CheckBox *> checkboxes;

	void _toggled(bool p_value);
};
