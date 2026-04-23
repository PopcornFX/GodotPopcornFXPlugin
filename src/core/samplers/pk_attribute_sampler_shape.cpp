//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_attribute_sampler_shape.h"

#include "godot_cpp/classes/box_shape3d.hpp"
#include "godot_cpp/classes/capsule_shape3d.hpp"
#include "godot_cpp/classes/concave_polygon_shape3d.hpp"
#include "godot_cpp/classes/convex_polygon_shape3d.hpp"
#include "godot_cpp/classes/cylinder_shape3d.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/physics_server3d.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/sphere_shape3d.hpp"

#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"

#include <pk_geometrics/include/ge_mesh_kdtree.h>
#include <pk_geometrics/include/ge_mesh_projection_accel.h>
#include <pk_geometrics/include/ge_mesh_resource.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>

namespace godot {

PKAttributeSamplerShape::~PKAttributeSamplerShape() {
	_clear_mesh_runtime_structs();
}

void PKAttributeSamplerShape::set_emitter(PKEmitter3D *p_emitter) {
	parent = p_emitter;
	_update_node(); // We most likely loaded without an emitter set, update node
	update_transforms();
}

void PKAttributeSamplerShape::update_transforms() {
	CParticleSamplerDescriptor_Shape_Default *shape_desc = static_cast<CParticleSamplerDescriptor_Shape_Default *>(desc.Get());
	if (shape_desc == nullptr) {
		return;
	}
	if (parent == nullptr || transform_mode == TRANSFORM_NONE) {
		shape_desc->m_WorldTr_Current = nullptr;
		shape_desc->m_WorldTr_Previous = nullptr;
	} else if (transform_mode == TRANSFORM_PARENT) {
		SSpawnTransformsPack pack = parent->get_transform_pack();
		shape_desc->m_WorldTr_Current = pack.m_WorldTr_Current;
		shape_desc->m_WorldTr_Previous = pack.m_WorldTr_Previous;
	} else if (transform_mode == TRANSFORM_NODE) {
		if (node != nullptr) {
			shape_desc->m_WorldTr_Current = &node_global_transform;
			shape_desc->m_WorldTr_Previous = &node_global_transform_previous;
		} else {
			shape_desc->m_WorldTr_Current = nullptr;
			shape_desc->m_WorldTr_Previous = nullptr;
		}
	} else {
		ERR_FAIL_MSG(vformat("Selected transform mode '%d' for PKAttributeSamplerShape is unimplemented", transform_mode));
	}
}

void PKAttributeSamplerShape::set_shape(Ref<Resource> p_shape) {
	if (p_shape.is_valid() && !p_shape->is_class(Shape3D::get_class_static()) && !p_shape->is_class(Mesh::get_class_static())) {
		ERR_FAIL_MSG(vformat("Cannot use %s as a shape to sample.", p_shape->get_class()));
	}
	if (shape.is_valid() && shape->is_connected("changed", callable_mp(this, &PKAttributeSamplerShape::_changed))) {
		shape->disconnect("changed", callable_mp(this, &PKAttributeSamplerShape::_changed));
	}

	shape = p_shape;
	if (shape.is_valid()) {
		if (!shape->is_connected("changed", callable_mp(this, &PKAttributeSamplerShape::_changed))) {
			shape->connect("changed", callable_mp(this, &PKAttributeSamplerShape::_changed));
		}
		_changed();
	} else {
		desc = nullptr;
		emit_changed();
	}
}

void PKAttributeSamplerShape::set_node_path(NodePath p_node_path) {
	node_path = p_node_path;
	_update_node();
}

void PKAttributeSamplerShape::set_transform_mode(TransformMode p_transform_mode) {
	transform_mode = p_transform_mode;
	update_transforms();
}

void PKAttributeSamplerShape::set_sample_mode(SampleMode p_sample_mode) {
	if (p_sample_mode == SAMPLE_SHAPE) {
		_set_node(nullptr);
	}
	if (p_sample_mode != SAMPLE_NODE && transform_mode == TRANSFORM_NODE) {
		set_transform_mode(TRANSFORM_NONE);
	}
	sample_mode = p_sample_mode;
	emit_changed();
}

void PKAttributeSamplerShape::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeSamplerShape, OBJECT, shape, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerShape, INT, transform_mode, PROPERTY_HINT_ENUM, "None,Parent,Node", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerShape, INT, sample_mode, PROPERTY_HINT_ENUM, "Shape,Node", PROPERTY_USAGE_STORAGE);
	BIND_BASIC_PROPERTY(PKAttributeSamplerShape, NODE_PATH, node_path, PROPERTY_HINT_NODE_PATH_VALID_TYPES, "MeshInstance3D", PROPERTY_USAGE_STORAGE);
	ClassDB::bind_method(D_METHOD("changed"), &PKAttributeSamplerShape::_changed);

	BIND_ENUM_CONSTANT(TRANSFORM_NONE);
	BIND_ENUM_CONSTANT(TRANSFORM_PARENT);
	BIND_ENUM_CONSTANT(TRANSFORM_NODE);

	BIND_ENUM_CONSTANT(SAMPLE_SHAPE);
	BIND_ENUM_CONSTANT(SAMPLE_NODE);
}

void PKAttributeSamplerShape::_changed() {
	if (node != nullptr && node->get_mesh() != shape) {
		set_shape(node->get_mesh());
		return;
	}
	if (shape->is_class(BoxShape3D::get_class_static())) {
		const Ref<BoxShape3D> box = shape;
		shape_desc = PK_NEW(CShapeDescriptor_Box(to_pk(box->get_size())));
	} else if (shape->is_class(CapsuleShape3D::get_class_static())) {
		const Ref<CapsuleShape3D> capsule = shape;
		shape_desc = PK_NEW(CShapeDescriptor_Capsule(capsule->get_radius(), capsule->get_height()));
	} else if (shape->is_class(CylinderShape3D::get_class_static())) {
		const Ref<CylinderShape3D> cylinder = shape;
		shape_desc = PK_NEW(CShapeDescriptor_Cylinder(cylinder->get_radius(), cylinder->get_height()));
	} else if (shape->is_class(SphereShape3D::get_class_static())) {
		const Ref<SphereShape3D> sphere = shape;
		shape_desc = PK_NEW(CShapeDescriptor_Sphere(sphere->get_radius()));
	} else if (shape->is_class(ConvexPolygonShape3D::get_class_static()) ||
			shape->is_class(ConcavePolygonShape3D::get_class_static()) ||
			shape->is_class(Mesh::get_class_static())) {
		const Ref<ConvexPolygonShape3D> convex = shape;
		if (convex.is_valid()) {
			const Ref<ArrayMesh> mesh = convex->get_debug_mesh(); // Despite its name, this is not editor-only and seems like the proper function to retrieve the mesh data
			Array array;
			for (int i = 0; i < mesh->get_surface_count(); i++) {
				if (mesh->surface_get_primitive_type(i) == Mesh::PRIMITIVE_TRIANGLES) {
					array = mesh->surface_get_arrays(i);
					break;
				}
			}
			ERR_FAIL_COND_MSG(array.is_empty(), "ConvexPolygonShape3D's debug mesh did not contain a surface with triangles.");

			mesh_vertices = PackedVector3Array(array[Mesh::ARRAY_VERTEX]);
			mesh_indices = PackedInt32Array(array[Mesh::ARRAY_INDEX]);
		} else if (shape->is_class(ConcavePolygonShape3D::get_class_static())) {
			const Ref<ConcavePolygonShape3D> concave = shape;
			mesh_vertices = concave->get_faces();
			mesh_indices = PackedInt32Array();
			mesh_indices.resize(mesh_vertices.size()); // TODO: once the todo below is done, merge vertices
			for (int i = 0; i < mesh_vertices.size(); i++) {
				mesh_indices[i] = i;
			}
		} else {
			const Ref<Mesh> mesh = shape;
			Array array;
			for (int i = 0; i < mesh->get_surface_count(); i++) {
				Dictionary surface = RenderingServer::get_singleton()->mesh_get_surface(mesh->get_rid(), i);
				if (int(surface["primitive"]) == Mesh::PRIMITIVE_TRIANGLES) {
					array = mesh->surface_get_arrays(i);
					break;
				}
			}
			if (array.size() == 0) {
				ERR_FAIL_MSG("PKAttributeSamplerShape: given mesh does not contain a triangles surface.");
			}

			mesh_vertices = PackedVector3Array(array[Mesh::ARRAY_VERTEX]);
			mesh_indices = PackedInt32Array(array[Mesh::ARRAY_INDEX]);
		}

		const int vertex_count = mesh_vertices.size();
		const int index_count = mesh_indices.size();
		ERR_FAIL_COND_MSG(vertex_count == 0, "PKAttributeSamplerShape: given mesh has invalid vertex buffer.");
		ERR_FAIL_COND_MSG(index_count == 0, "PKAttributeSamplerShape: given mesh has invalid index buffer.");

		CShapeDescriptor_Mesh *desc = PK_NEW(CShapeDescriptor_Mesh);
		if (!PKGD_VERIFY(desc != nullptr)) {
			return;
		}
		SMeshProxy proxy;
		proxy.m_PrimitiveType = CMeshIStream::Triangles;
		proxy.m_IndexFormat = CMeshIStream::U32Indices;
		proxy.m_Indices = mesh_indices.ptr();
		proxy.m_IndexCount = index_count;
		proxy.m_VertexCount = vertex_count;
		proxy.m_Positions = TStridedMemoryView<const CFloat3>(reinterpret_cast<const CFloat3 *>(mesh_vertices.ptr()), vertex_count);
		desc->SetMeshProxy(proxy);

		_setup_mesh_runtime_structs(desc);

		shape_desc = desc;
	} else {
		ERR_FAIL_MSG(vformat("Could not create PKAttributeSamplerShape from shape %s", shape->get_class()));
	}

	desc = PK_NEW(CParticleSamplerDescriptor_Shape_Default(shape_desc.Get()));
	update_transforms();
	emit_changed();
}

void PKAttributeSamplerShape::_disconnect_changed() {
	if (shape.is_valid()) {
		shape->disconnect("changed", callable_mp(this, &PKAttributeSamplerShape::_changed));
	}
}

void PKAttributeSamplerShape::_physics_process() {
	if (node != nullptr) {
		node_global_transform_previous = node_global_transform;
		node_global_transform = to_pk(node->get_global_transform());
	}
}

void PKAttributeSamplerShape::_ready() {
	ready = true;
	_update_node();
}

void PKAttributeSamplerShape::_update_node() {
	if (!ready) {
		return;
	}

	if (node_path.is_empty()) {
		if (node != nullptr) {
			_set_node(nullptr);
		}
		return;
	}

	ERR_FAIL_NULL(parent);
	Node *new_node = parent->get_node<Node>(node_path);
	if (new_node == node) { // Node moved.
		return;
	}
	ERR_FAIL_NULL_MSG(new_node, vformat("Node path '%s' given to PKAttributeSamplerShape is not valid.", node_path.get_concatenated_names()));
	MeshInstance3D *as_mesh_instance = cast_to<MeshInstance3D>(new_node);
	ERR_FAIL_NULL_MSG(as_mesh_instance, vformat("Node path '%s' given to PKAttributeSamplerShape is not a MeshInstance3D.", node_path.get_concatenated_names()));

	_set_node(as_mesh_instance);
}

void PKAttributeSamplerShape::_set_node(MeshInstance3D *p_new_node) {
	node = p_new_node;
	_changed();
}

void PKAttributeSamplerShape::_setup_mesh_runtime_structs(CShapeDescriptor_Mesh *p_desc) {
	_clear_mesh_runtime_structs();

	mesh_surface_sampling_struct = PK_NEW(CMeshSurfaceSamplerStructuresRandom);
	if (!PKGD_VERIFY(mesh_surface_sampling_struct != nullptr)) {
		return;
	}
	mesh_surface_sampling_struct->Build(p_desc->MeshProxy());
	p_desc->SetSamplingStructs(mesh_surface_sampling_struct, nullptr); // TODO: Cache & serialise these in a central manager
	mesh_kd_tree = PK_NEW(CMeshKdTree);
	if (!PKGD_VERIFY(mesh_kd_tree != nullptr)) {
		return;
	}
	SMeshKdTreeBuildConfig kdTreeConfig = SMeshKdTreeBuildConfig(true); // Use fast KDTree for now. Switch to HQ one when these accel structs are cached.
	mesh_kd_tree->Build(p_desc->MeshProxy(), kdTreeConfig);
	p_desc->SetKdTree(mesh_kd_tree);
	mesh_projection = PK_NEW(CMeshProjection);
	if (!PKGD_VERIFY(mesh_projection != nullptr)) {
		return;
	}
	CMeshProjection::SConfig config = CMeshProjection::SConfig::Default();
	mesh_projection->Build(p_desc->MeshProxy(), mesh_kd_tree, config);
	p_desc->SetProjectionStructs(mesh_projection);
}

void PKAttributeSamplerShape::_clear_mesh_runtime_structs() {
	PK_SAFE_DELETE(mesh_surface_sampling_struct);
	PK_SAFE_DELETE(mesh_kd_tree);
	PK_SAFE_DELETE(mesh_projection);
}

} // namespace godot

VARIANT_ENUM_CAST(PKAttributeSamplerShape::TransformMode);
VARIANT_ENUM_CAST(PKAttributeSamplerShape::SampleMode);
