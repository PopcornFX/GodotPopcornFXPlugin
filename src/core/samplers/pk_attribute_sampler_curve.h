#pragma once

#include "godot_cpp/classes/curve.hpp"
#include "godot_cpp/classes/gradient.hpp"

#include "pk_attribute_sampler.h"

#include <pk_particles/include/ps_samplers_curve.h>

namespace godot {

class PKAttributeSamplerCurve : public PKAttributeSampler {
	GDCLASS(PKAttributeSamplerCurve, PKAttributeSampler);

public:
	PKAttributeSamplerCurve();
	PKAttributeSamplerCurve(const CResourceDescriptor *p_resource_descriptor);

	void set_order(int p_order);
	int get_order() const { return order; }

	void set_curve(int p_index, Ref<Curve> p_curve);
	Ref<Curve> get_curve(int p_index) const;
	String get_curve_letter(int p_index) const;

	void set_x(Ref<Curve> p_curve) { return set_curve(0, p_curve); }
	Ref<Curve> get_x() const { return x; }
	void set_y(Ref<Curve> p_curve) { return set_curve(1, p_curve); }
	Ref<Curve> get_y() const { return y; }
	void set_z(Ref<Curve> p_curve) { return set_curve(2, p_curve); }
	Ref<Curve> get_z() const { return z; }
	void set_w(Ref<Curve> p_curve) { return set_curve(3, p_curve); }
	Ref<Curve> get_w() const { return w; }

	void set_edit_as_gradient(bool p_edit_as_gradient);
	bool get_edit_as_gradient() const { return edit_as_gradient; }
	void set_gradient(Ref<Gradient> p_gradient);
	Ref<Gradient> get_gradient() const { return gradient; }

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;
	virtual PackedStringArray _get_configuration_warnings() const override;
	virtual void _update_from_descriptor(const CResourceDescriptor *p_descriptor) override;

private:
	void _create_default_curves();
	static Ref<Curve> _create_curve_from_default(const Ref<Curve> p_default);
	void _set_curve_raw(int p_index, Ref<Curve> p_curve);
	bool _check_curves();
	void _enforce_curve_size(Ref<Curve> p_curve, int p_point_count);
	void _changed();
	void _update_from_gradient();

	int order = 0;
	CCurveDescriptor *curve_desc = nullptr;
	Ref<Curve> x;
	Ref<Curve> y;
	Ref<Curve> z;
	Ref<Curve> w;

	bool edit_as_gradient;
	Ref<Gradient> gradient;
	PackedFloat32Array gradient_offsets;
	PackedColorArray gradient_colors;
};

} //namespace godot
