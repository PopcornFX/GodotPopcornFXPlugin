//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_attribute_sampler_curve.h"

#include "godot_cpp/classes/curve2d.hpp"

#include "core/pk_attribute_list.h"
#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"

#include <pk_particles/include/ps_samplers_curve.h>

namespace godot {
PKAttributeSamplerCurve::PKAttributeSamplerCurve() {
	curve_desc = PK_NEW(CCurveDescriptor);
	curve_desc->m_Interpolator = CInterpolableVectorArray::Interpolator_Hermite;

	edit_as_gradient = false;
	gradient.instantiate();
	gradient->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_update_from_gradient));
}

PKAttributeSamplerCurve::PKAttributeSamplerCurve(const CResourceDescriptor *p_resource_descriptor) : PKAttributeSamplerCurve() {
	const CResourceDescriptor_Curve *resource_desc = HBO::Cast<const CResourceDescriptor_Curve>(p_resource_descriptor);
	if (PKGD_VERIFY(resource_desc != nullptr)) {
		set_order(dynamic_cast<const CResourceDescriptor_Curve *>(p_resource_descriptor)->Order());
		_create_default_curves();
	}
}

void PKAttributeSamplerCurve::set_order(int p_order) {
	curve_desc->m_Order = p_order;
	order = p_order;
}

void PKAttributeSamplerCurve::set_curve(int p_index, Ref<Curve> p_curve) {
	ERR_FAIL_COND_MSG(p_index >= order, vformat("PKAttributeSamplerCurve curve index out of bounds: %d", p_index));
	Ref<Curve> current = get_curve(p_index);

	const Callable changed_callable = callable_mp(this, &PKAttributeSamplerCurve::_changed);

	if (current.is_valid() && current->is_connected("changed", changed_callable)) {
		current->disconnect("changed", changed_callable);
	}

	_set_curve_raw(p_index, p_curve);
	if (p_curve.is_valid()) {
		if (!p_curve->is_connected("changed", changed_callable)) {
			p_curve->connect("changed", changed_callable);
		}
		_changed();
	} else {
		emit_changed();
	}
}

Ref<Curve> PKAttributeSamplerCurve::get_curve(int p_index) const {
	switch (p_index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		default:
			return nullptr;
	}
}

String PKAttributeSamplerCurve::get_curve_letter(int p_index) const {
	switch (p_index) {
		case 0:
			return "X";
		case 1:
			return "Y";
		case 2:
			return "Z";
		case 3:
			return "W";
		default:
			return "";
	}
}

void PKAttributeSamplerCurve::set_edit_as_gradient(bool p_edit_as_gradient) {
	this->edit_as_gradient = p_edit_as_gradient;
	notify_property_list_changed();
	if (!_check_curves()) {
		emit_changed();
		return;
	}
	const int point_count = x->get_point_count();
	// Clear gradient
	gradient_offsets.resize(point_count);
	gradient_colors.resize(point_count);

	for (int i = 0; i < order; i++) {
		const Ref<Curve> curve = get_curve(i);
		if (curve.is_valid()) {
			_enforce_curve_size(curve, point_count);
			for (int j = 0; j < point_count; j++) {
				const Vector2 pos = curve->get_point_position(j);
				gradient_offsets[j] = pos.x;
				gradient_colors[j] = Color();
				gradient_colors[j].components[i] = CLAMP(pos.y, 0.f, 1.f);
			}
		}
	}

	gradient->disconnect("changed", callable_mp(this, &PKAttributeSamplerCurve::_update_from_gradient));
	gradient->set_offsets(gradient_offsets);
	gradient->set_colors(gradient_colors);
	gradient->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_update_from_gradient));
}

void PKAttributeSamplerCurve::set_gradient(Ref<Gradient> p_gradient) {
	gradient = p_gradient;
	if (gradient.is_valid()) {
		gradient->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_update_from_gradient));
	}
	_update_from_gradient();
}

void PKAttributeSamplerCurve::_bind_methods() {
	// Sync with _get_property_list
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, INT, order, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, BOOL, edit_as_gradient, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, OBJECT, gradient, PROPERTY_HINT_RESOURCE_TYPE, "Gradient", PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, OBJECT, x, PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, OBJECT, y, PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, OBJECT, z, PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerCurve, OBJECT, w, PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
}

void PKAttributeSamplerCurve::_get_property_list(List<PropertyInfo> *p_list) const {
	if (edit_as_gradient) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "gradient", PROPERTY_HINT_RESOURCE_TYPE, "Gradient", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ALWAYS_DUPLICATE));
		return;
	}
	if (order > 0) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "x", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ALWAYS_DUPLICATE));
	}
	if (order > 1) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "y", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ALWAYS_DUPLICATE));
	}
	if (order > 2) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "z", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ALWAYS_DUPLICATE));
	}
	if (order > 3) {
		p_list->push_back(PropertyInfo(Variant::OBJECT, "w", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_ALWAYS_DUPLICATE));
	}
}

PackedStringArray PKAttributeSamplerCurve::_get_configuration_warnings() const {
	PackedStringArray warnings;
	for (int i = 0; i < order; ++i) {
		const Ref<Curve> curve = get_curve(i);
		if (curve.is_null()) {
			warnings.push_back("Curve " + get_curve_letter(i) + " is null");
		}
	}
	if (x.is_null()) {
		return warnings;
	}
	const int expected_point_count = x->get_point_count();
	for (int i = 1; i < order; ++i) {
		const Ref<Curve> curve = get_curve(i);
		if (curve.is_null()) {
			continue;
		}
		const int point_count = curve->get_point_count();
		if (point_count != expected_point_count) {
			warnings.push_back(vformat("Curve %s has %d points, expected the same number as Curve X (%d", get_curve_letter(i), point_count, expected_point_count));
		}
	}

	return warnings;
}

void PKAttributeSamplerCurve::_update_from_descriptor(const CResourceDescriptor *p_descriptor) {
	PKAttributeSampler::_update_from_descriptor(p_descriptor);
	set_order(dynamic_cast<const CResourceDescriptor_Curve *>(p_descriptor)->Order());
	_create_default_curves();
	_changed();
}

void PKAttributeSamplerCurve::_create_default_curves() {
	for (int i = 0; i < order; ++i) {
		Ref<Curve> curve = get_curve(i);
		if (curve.is_null()) {
			curve = _create_curve_from_default(x);
			curve->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_changed));
			_set_curve_raw(i, curve);
		}
	}
}

Ref<Curve> PKAttributeSamplerCurve::_create_curve_from_default(const Ref<Curve> p_default) {
	Ref<Curve> new_curve = memnew(Curve);
	if (p_default.is_valid()) {
		for (int i = 0; i < p_default->get_point_count(); ++i) {
			new_curve->add_point(p_default->get_point_position(i));
		}
	}

	return new_curve;
}

void PKAttributeSamplerCurve::_changed() {
	if (!_check_curves()) {
		emit_changed();
		return;
	}
	const int point_count = x->get_point_count();
	curve_desc->m_Times.Resize(point_count);
	curve_desc->m_FloatValues.Resize(point_count * order);
	curve_desc->m_FloatTangents.Resize(point_count * order * 2);

	{
		const Ref<Curve> curve = get_curve(0);
		for (int j = 0; j < point_count; j++) {
			curve_desc->m_Times[j] = curve->get_point_position(j).x;
		}
	}
	for (int i = 0; i < order; i++) {
		const Ref<Curve> curve = get_curve(i);
		curve->disconnect("changed", callable_mp(this, &PKAttributeSamplerCurve::_changed));
		if (curve.is_valid()) {
			// Force godot curve alignment
			_enforce_curve_size(curve, point_count);

			// Fill popcorn curve
			float previous_time = 0.0f;
			for (int j = 0; j < point_count; j++) {
				const Vector2 pos = curve->get_point_position(j);
				if (pos.x != curve_desc->m_Times[j]) {
					curve->set_point_offset(j, curve_desc->m_Times[j]);
				}
				curve_desc->m_FloatValues[order * j + i] = pos.y;

				float left_tangent = curve->get_point_left_tangent(j);
				left_tangent = left_tangent * (pos.x - previous_time);
				curve_desc->m_FloatTangents[order * j * 2 + i] = left_tangent;

				float right_tangent = curve->get_point_right_tangent(j);
				float next_time = curve->get_max_domain();
				if (point_count > j + 1) {
					const Vector2 next_pos = curve->get_point_position(j + 1);
					next_time = next_pos.x;
				}
				right_tangent = right_tangent * (next_time - pos.x);
				curve_desc->m_FloatTangents[order * j * 2 + order + i] = right_tangent;

				previous_time = pos.x;
			}
		}
		curve->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_changed));
	}
	curve_desc->RecomputeParametricDomain();
	if (desc == nullptr) {
		desc = PK_NEW(CParticleSamplerDescriptor_Curve_Default(curve_desc));
	}
	reinterpret_cast<CParticleSamplerDescriptor_Curve_Default *>(desc.Get())->RebuildData();

	emit_changed();
}

void PKAttributeSamplerCurve::_set_curve_raw(int p_index, Ref<Curve> p_curve) {
	switch (p_index) {
		case 0:
			x = p_curve;
			break;
		case 1:
			y = p_curve;
			break;
		case 2:
			z = p_curve;
			break;
		case 3:
			w = p_curve;
			break;
	}
}

bool PKAttributeSamplerCurve::_check_curves() {
	if (x.is_null()) {
		return false;
	}
	for (int i = 1; i < order; i++) {
		const Ref<Curve> curve = get_curve(i);
		if (curve.is_null()) {
			return false;
		}
	}
	return true;
}

void PKAttributeSamplerCurve::_enforce_curve_size(Ref<Curve> p_curve, int p_point_count) {
	const int current_point_count = p_curve->get_point_count();
	if (current_point_count > p_point_count) {
		p_curve->set_point_count(p_point_count);
	} else {
		for (int j = 0; j < p_point_count - current_point_count; j++) {
			p_curve->add_point(Vector2(p_curve->get_max_domain(), p_curve->get_max_value()));
		}
	}
}

void PKAttributeSamplerCurve::_update_from_gradient() {
	// Clear all curves
	for (int i = 0; i < order; ++i) {
		Ref<Curve> curve = get_curve(i);
		if (curve.is_valid()) {
			curve->disconnect("changed", callable_mp(this, &PKAttributeSamplerCurve::_changed));
			curve->clear_points();
		} else {
			curve.instantiate();
			_set_curve_raw(i, curve);
		}
	}
	if (gradient.is_valid()) {
		for (int i = 0; i < gradient->get_point_count(); ++i) {
			const float offset = gradient->get_offset(i);
			const Color color = gradient->get_color(i);
			x->add_point(Vector2(offset, color.components[0]));
			if (order > 1) {
				y->add_point(Vector2(offset, color.components[1]));
			}
			if (order > 2) {
				z->add_point(Vector2(offset, color.components[2]));
			}
			if (order > 3) {
				w->add_point(Vector2(offset, color.components[3]));
			}
		}
	}

	for (int i = 0; i < order; ++i) {
		get_curve(i)->connect("changed", callable_mp(this, &PKAttributeSamplerCurve::_changed));
	}

	_changed();
}
} //namespace godot
