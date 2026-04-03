//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/resource.hpp"

#include "pk_attribute_sampler.h"

#include <pk_geometrics/include/ge_coordinate_frame.h>
#include <pk_particles/include/ps_samplers_shape.h>

namespace godot {

class PKAttributeSamplerShape : public PKAttributeSampler {
	GDCLASS(PKAttributeSamplerShape, PKAttributeSampler);

public:
	enum TransformMode {
		TRANSFORM_NONE,
		TRANSFORM_PARENT,
		TRANSFORM_NODE,
	};

	enum SampleMode {
		SAMPLE_SHAPE,
		SAMPLE_NODE,
	};

	PKAttributeSamplerShape() = default;
	PKAttributeSamplerShape(const PopcornFX::CResourceDescriptor *) {}
	virtual ~PKAttributeSamplerShape();

	virtual void set_emitter(PKEmitter3D *p_emitter) override;
	void update_transforms();

	Ref<Resource> get_shape() const { return shape; }
	void set_shape(Ref<Resource> p_shape);
	NodePath get_node_path() const {
		if (node != nullptr) {
			return parent->get_path_to(node);
		}
		return "";
	}
	void set_node_path(NodePath p_node_path);
	TransformMode get_transform_mode() const { return transform_mode; }
	void set_transform_mode(TransformMode p_transform_mode);
	SampleMode get_sample_mode() const { return sample_mode; }
	void set_sample_mode(SampleMode p_sample_mode);

	PShapeDescriptor shape_desc = nullptr;

protected:
	static void _bind_methods();
	void _changed();
	void _disconnect_changed();
	virtual void _physics_process() override;
	virtual void _ready() override;
	void _update_node();
	void _setup_mesh_runtime_structs(CShapeDescriptor_Mesh *desc);
	void _clear_mesh_runtime_structs();

	bool ready = false;
	Ref<Resource> shape;
	MeshInstance3D *node = nullptr;
	NodePath node_path;
	TransformMode transform_mode = TRANSFORM_NONE;
	SampleMode sample_mode = SAMPLE_SHAPE;

	CFloat4x4 node_global_transform_previous = CFloat4x4::ZERO;
	CFloat4x4 node_global_transform = CFloat4x4::ZERO;

	PackedVector3Array mesh_vertices;
	PackedInt32Array mesh_indices;

	CMeshSurfaceSamplerStructuresRandom *mesh_surface_sampling_struct = nullptr;
	CMeshKdTree *mesh_kd_tree = nullptr;
	CMeshProjection *mesh_projection = nullptr;
};

} // namespace godot
