//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_resource_handler_mesh.h"

#include "godot_cpp/classes/editor_interface.hpp"
#include "godot_cpp/classes/material.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

#include "integration/engine/pk_file_system_controller.h"
#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"
#include "pk_manager.h"

#include <pk_geometrics/include/ge_mesh_kdtree.h>
#include <pk_geometrics/include/ge_mesh_sampler_accel.h>
#include <pk_kernel/include/kr_refcounted_buffer.h>

namespace godot {
namespace {
PKResourceHandlerMesh *g_resource_handler_mesh_godot = nullptr;
} //namespace

PKResourceMeshEntry::~PKResourceMeshEntry() {
}

void PKResourceHandlerMesh::startup() {
	PKGD_ASSERT(g_resource_handler_mesh_godot == nullptr);
	g_resource_handler_mesh_godot = memnew(PKResourceHandlerMesh);
	if (!PK_VERIFY(g_resource_handler_mesh_godot != nullptr)) {
		return;
	}
	PopcornFX::Resource::DefaultManager()->RegisterHandler<CResourceMesh>(g_resource_handler_mesh_godot);
}

void PKResourceHandlerMesh::shutdown() {
	if (PK_VERIFY(g_resource_handler_mesh_godot != nullptr)) {
		PopcornFX::Resource::DefaultManager()->UnregisterHandler<CResourceMesh>(g_resource_handler_mesh_godot);
		memdelete(g_resource_handler_mesh_godot);
		g_resource_handler_mesh_godot = nullptr;
	}
}

CResourceMesh *PKResourceHandlerMesh::new_from_gd_meshes(const Vector<Pair<Ref<Mesh>, Transform3D>> &p_meshes) {
	if (p_meshes.is_empty()) {
		return nullptr;
	}
	CResourceMesh *new_mesh = PK_NEW(CResourceMesh);
	if (!PK_VERIFY(new_mesh != nullptr)) {
		return nullptr;
	}
	Vector<RawMeshData> meshes = _raw_from_gd_meshes(p_meshes);

	for (const RawMeshData &mesh_data : meshes) {
		for (int i = 0; i < mesh_data.surfaces.size(); i++) {
			const RawSurfaceData &surface_data = mesh_data.surfaces[i];
			const int vertex_count = surface_data.vertices.size();
			const int index_count = surface_data.indices.size();
			ERR_FAIL_COND_V_MSG(vertex_count == 0, nullptr, "PKResourceHandlerMesh: given mesh has an empty vertex buffer.");
			ERR_FAIL_COND_V_MSG(index_count == 0, nullptr, "PKResourceHandlerMesh: given mesh has an empty index buffer.");
			const bool has_normals = !surface_data.normals.is_empty();
			const bool has_tangents = !surface_data.tangents.is_empty();
			const bool has_colors = !surface_data.colors.is_empty();
			const bool has_UVs = !surface_data.UVs.is_empty();
			const bool has_UV2s = !surface_data.UV2s.is_empty();

			SVertexDeclaration vertex_declaration;
			if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::Ordinal_Position, SVStreamCode::Element_Float3)))) {
				return nullptr;
			}
			if (has_normals) {
				if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::Ordinal_Normal, SVStreamCode::Element_Float3)))) {
					return nullptr;
				}
			}
			if (has_tangents) {
				if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::Ordinal_Tangent, SVStreamCode::Element_Float4)))) {
					return nullptr;
				}
			}
			if (has_colors) {
				if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::Ordinal_Color, SVStreamCode::Element_Float4)))) {
					return nullptr;
				}
			}
			if (has_UVs) {
				if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::Ordinal_Texcoord, SVStreamCode::Element_Float2)))) {
					return nullptr;
				}
			}
			if (has_UV2s) {
				if (!PKGD_VERIFY(vertex_declaration.AddStreamCodeIFN(SVStreamCode(CVStreamSemanticDictionnary::UvStreamToOrdinal(1), SVStreamCode::Element_Float2)))) {
					return nullptr;
				}
			}

			CMeshTriangleBatch batch = CMeshTriangleBatch(CMeshIStream::U32Indices, CMeshIStream::Triangles, vertex_declaration);
			batch.m_IStream.Resize(index_count);
			memcpy(batch.m_IStream.StreamForWriting<uint32_t>(), surface_data.indices.ptr(), batch.m_IStream.StreamSize());
			batch.m_VStream.Resize(vertex_count);
			batch.m_VStream.ImportPositions(TStridedMemoryView<const CFloat3>(reinterpret_cast<const CFloat3 *>(surface_data.vertices.ptr()), vertex_count));
			if (has_normals) {
				batch.m_VStream.ImportNormals(TStridedMemoryView<const CFloat3>(reinterpret_cast<const CFloat3 *>(surface_data.normals.ptr()), vertex_count));
			}
			if (has_tangents) {
				batch.m_VStream.ImportTangents(TStridedMemoryView<const CFloat4>(reinterpret_cast<const CFloat4 *>(surface_data.tangents.ptr()), vertex_count));
			}
			if (has_colors) {
				batch.m_VStream.ImportColors(TStridedMemoryView<const CFloat4>(reinterpret_cast<const CFloat4 *>(surface_data.colors.ptr()), vertex_count));
			}
			if (has_UVs) {
				batch.m_VStream.ImportTexcoords(TStridedMemoryView<const CFloat2>(reinterpret_cast<const CFloat2 *>(surface_data.UVs.ptr()), vertex_count));
			}
			if (has_UV2s) {
				batch.m_VStream.ImportAbstractStream(CVStreamSemanticDictionnary::UvStreamToOrdinal(1), TStridedMemoryView<const CFloat2>(reinterpret_cast<const CFloat2 *>(surface_data.UV2s.ptr()), vertex_count));
			}

			CMeshNew *mesh_new = PK_NEW(CMeshNew(batch));

			mesh_new->BuildKdTree(SMeshKdTreeBuildConfig(true), true); // FIXME: Using fast build until this is cached
			// Setup manually to pass forceRebuild, ignoring the warning about runtime accel struct generation
#if (PK_GEOMETRICS_BUILD_MESH_SAMPLER_SURFACE != 0)
			mesh_new->SetupDefaultSurfaceSamplingAccelStructsIFN(true);
#endif
#if (PK_GEOMETRICS_BUILD_MESH_SAMPLER_VOLUME != 0)
			mesh_new->SetupDefaultVolumeSamplingAccelStructsIFN(true);
#endif
#if (PK_GEOMETRICS_BUILD_MESH_SAMPLER_SURFACE != 0)
			mesh_new->SetupDefaultSurfaceSamplingAccelStructsUVIFN(SMeshUV2PCBuildConfig(), true);
#endif
			// Call this anyway in case we missed something
			mesh_new->SetupRuntimeStructsIFN(true, false);

			// Set mesh name
			String name = mesh_data.name + String("_");
			if (surface_data.name.is_empty()) {
				name += itos(i);
			} else {
				name += surface_data.name;
			}
			mesh_new->SetName(to_pk(name));

			// Try to match an LOD level to the name
			int lod = 0;
			PackedStringArray parts = mesh_data.name.to_lower().split("lod");
			if (parts.size() > 1) {
				const String &last_part = parts[parts.size() - 1];
				if (last_part.is_valid_int()) {
					lod = last_part.to_int();
				}
			}

			new_mesh->AddBatch("mat", mesh_new, lod);
		}
	}

	return new_mesh;
}

CResourceMesh *PKResourceHandlerMesh::new_from_gd_resource(const Ref<Resource> p_resource) {
	ERR_FAIL_NULL_V(p_resource, nullptr);

	if (p_resource->is_class(PackedScene::get_class_static())) {
		return new_from_gd_meshes(_meshes_from_packed_scene(p_resource));
	}
	if (p_resource->is_class(Mesh::get_class_static())) {
		return new_from_gd_meshes({ { p_resource, Transform3D(Basis(Quaternion(), Vector3(100, 100, 100))) } });
	}

	ERR_FAIL_V_MSG(nullptr,
			vformat("Unsupported resource '%s' for creation of PopcornFX mesh resource",
					p_resource->get_path()));
}

Ref<ArrayMesh> PKResourceHandlerMesh::gd_merge_packed_scene_mesh(const Ref<PackedScene> p_packed_scene) {
	ERR_FAIL_NULL_V(p_packed_scene, nullptr);

	Vector<RawMeshData> meshes = _raw_from_gd_meshes(_meshes_from_packed_scene(p_packed_scene));

	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	for (const RawMeshData &mesh_data : meshes) {
		for (const RawSurfaceData &surface_data : mesh_data.surfaces) {
			Array surface_array;
			surface_array.resize(Mesh::ARRAY_MAX);

			surface_array[Mesh::ARRAY_VERTEX] = surface_data.vertices;
			surface_array[Mesh::ARRAY_INDEX] = surface_data.indices;

			if (!surface_data.normals.is_empty()) {
				surface_array[Mesh::ARRAY_NORMAL] = surface_data.normals;
			}
			if (!surface_data.tangents.is_empty()) {
				surface_array[Mesh::ARRAY_TANGENT] = surface_data.tangents;
			}
			if (!surface_data.colors.is_empty()) {
				surface_array[Mesh::ARRAY_COLOR] = surface_data.colors;
			}
			if (!surface_data.UVs.is_empty()) {
				surface_array[Mesh::ARRAY_TEX_UV] = surface_data.UVs;
			}
			if (!surface_data.UV2s.is_empty()) {
				surface_array[Mesh::ARRAY_TEX_UV2] = surface_data.UV2s;
			}

			mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_array);
		}
	}
	return mesh;
}

void *PKResourceHandlerMesh::Load(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CString &p_resource_path,
		bool p_path_not_virtual,
		const SResourceLoadCtl &p_load_ctl,
		CMessageStream &p_load_report,
		SAsyncLoadStatus *p_async_load_status) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());

	PKFileSystemController *file_system = static_cast<PKFileSystemController *>(p_resource_manager->FileController());
	String godot_path = file_system->sanitize_path(to_gd(p_resource_path), !p_path_not_virtual);

	if (!PKGD_VERIFY(!godot_path.begins_with("::"))) {
		ERR_FAIL_V_MSG(nullptr, vformat("Can't load temporary mesh resources. ('%s')", godot_path));
	}

	if (godot_path.is_empty()) {
		if (p_async_load_status != nullptr) {
			p_async_load_status->m_Resource = nullptr;
			p_async_load_status->m_Done = true;
			p_async_load_status->m_Progress = 1.0f;
		}
		ERR_FAIL_V_MSG(nullptr, "PKResourceHandlerMesh: given resource path is empty.");
	}

	// Create entry with an empty godot mesh, (potentially) defer call to fill said mesh and update popcorn resource
	Ref<PKResourceMeshEntry> entry = memnew(PKResourceMeshEntry);
	entry->pk_resource = PK_NEW(CResourceMesh);
	RenderingServer::get_singleton()->call_on_render_thread(callable_mp_static(&PKResourceHandlerMesh::_deferred_update_mesh).bind(entry).bind(godot_path));
	if (p_async_load_status != nullptr) {
		p_async_load_status->m_Resource = nullptr;
		p_async_load_status->m_Done = true;
		p_async_load_status->m_Progress = 1.0f;
	}
	return entry->pk_resource.Get();
}

void *PKResourceHandlerMesh::Load(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CFilePackPath &p_resource_path,
		const SResourceLoadCtl &p_load_ctl,
		CMessageStream &p_load_report,
		SAsyncLoadStatus *p_async_load_status) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());
	return Load(p_resource_manager, p_resource_type_id, p_resource_path.Path(), false, p_load_ctl, p_load_report, p_async_load_status);
}

void PKResourceHandlerMesh::Unload(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		void *p_resource) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());
}

void PKResourceHandlerMesh::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		void *p_resource,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());
}

void PKResourceHandlerMesh::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CString &p_resource_path,
		bool p_path_not_virtual,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());
}

void PKResourceHandlerMesh::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CFilePackPath &p_resource_path,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CResourceMesh>::ResourceTypeID());
}

Vector<RawMeshData> PKResourceHandlerMesh::_raw_from_gd_meshes(const Vector<Pair<Ref<Mesh>, Transform3D>> &p_meshes) {
	Vector<RawMeshData> meshes;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (const Pair<Ref<Mesh>, Transform3D> &pair : p_meshes) {
		const Ref<Mesh> mesh = pair.first;
		RawMeshData mesh_data;
		mesh_data.name = mesh->get_name();
		for (int i = 0; i < mesh->get_surface_count(); i++) {
			RawSurfaceData surface_data;

			Ref<Material> material = mesh->surface_get_material(i);
			if (material.is_valid()) {
				surface_data.name = material->get_name();
			}

			Dictionary surface = rs->mesh_get_surface(mesh->get_rid(), i);
			if (int(surface["primitive"]) != Mesh::PRIMITIVE_TRIANGLES) {
				continue;
			}
			Array array = mesh->surface_get_arrays(i);

			surface_data.vertices = array[Mesh::ARRAY_VERTEX];
			surface_data.indices = array[Mesh::ARRAY_INDEX];
			surface_data.normals = array[Mesh::ARRAY_NORMAL];
			surface_data.tangents = array[Mesh::ARRAY_TANGENT];
			surface_data.colors = array[Mesh::ARRAY_COLOR];
			surface_data.UVs = array[Mesh::ARRAY_TEX_UV];
			surface_data.UV2s = array[Mesh::ARRAY_TEX_UV2];

			const int vertex_count = surface_data.vertices.size();
			const int index_count = surface_data.indices.size();
			ERR_FAIL_COND_V_MSG(vertex_count == 0, meshes, "PKResourceHandlerMesh: given mesh has an empty vertex buffer.");
			ERR_FAIL_COND_V_MSG(index_count == 0, meshes, "PKResourceHandlerMesh: given mesh has an empty index buffer.");

			// Apply transforms
			const Transform3D transform = pair.second;
			for (int i = 0; i < vertex_count; i++) {
				surface_data.vertices[i] = transform.xform(surface_data.vertices[i]);
			}
			for (int i = 0; i < surface_data.normals.size(); i++) {
				surface_data.normals[i] = transform.basis.xform(surface_data.normals[i]).normalized();
			}
			float *tangent_ptrw = surface_data.tangents.ptrw();
			for (int i = 0; i < surface_data.tangents.size(); i += 4) {
				Vector3 *tangent = reinterpret_cast<Vector3 *>(tangent_ptrw + i);
				*tangent = transform.basis.xform(*tangent).normalized();
			}

			mesh_data.surfaces.push_back(surface_data);
		}
		if (mesh_data.surfaces.size() == 0) {
			ERR_FAIL_V_MSG({}, "PKResourceHandlerMesh: given mesh does not contain a triangles surface.");
		}
		meshes.push_back(mesh_data);
	}

	return meshes;
}

Vector<Pair<Ref<Mesh>, Transform3D>> PKResourceHandlerMesh::_meshes_from_packed_scene(const Ref<PackedScene> p_packed_scene) {
	if (p_packed_scene.is_null()) {
		return {};
	}
	Node *scene_node = p_packed_scene->instantiate();
	ERR_FAIL_NULL_V(scene_node, {});

	// Global transforms aren't available when a node isn't in a tree, and we may not have access to a tree.
	// Compute global transforms ourselves
	HashMap<const Node3D *, Transform3D> transform_map;
	Array all_children = scene_node->find_children("", "Node3D");
	for (const Object *object : all_children) {
		const Node3D *node = Object::cast_to<Node3D>(object);
		if (!node->get_parent_node_3d()) {
			transform_map[node] = node->get_transform();
		} else {
			transform_map[node] = transform_map[node->get_parent_node_3d()] * node->get_transform();
		}
	}

	// Get all meshes in the scene
	const TypedArray<MeshInstance3D> mesh_instances = scene_node->find_children("", "MeshInstance3D");
	Vector<Pair<Ref<Mesh>, Transform3D>> meshes;
	for (int i = 0; i < mesh_instances.size(); i++) {
		MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(mesh_instances[i]);
		if (mesh_instance != nullptr) {
			Ref<Mesh> mesh = mesh_instance->get_mesh();
			if (mesh.is_valid()) {
				meshes.push_back(Pair(mesh, transform_map[mesh_instance]));
			}
		}
	}

	// Cleanup
	scene_node->queue_free();

	return meshes;
}

void PKResourceHandlerMesh::_update_mesh(const Ref<PKResourceMeshEntry> p_entry) {
	PResourceMesh resource = new_from_gd_resource(p_entry->gd_resource);

	PResourceMesh dst_resource = p_entry->pk_resource; // strong ref in case final 'm_OnReloaded' clears all refs and deletes resource

	dst_resource->m_OnReloading(dst_resource.Get());

	if (resource != nullptr) {
		dst_resource->Swap(*resource);
	} else {
		dst_resource->Clear();
	}

	dst_resource->m_OnReloaded(dst_resource.Get());

	if (resource == nullptr) {
		ERR_FAIL_MSG(vformat("PKPKResourceHandlerMesh: Failed loading mesh resource \"%s\"", p_entry->gd_resource->get_path()));
	}
}

void PKResourceHandlerMesh::_deferred_update_mesh(const String &p_path, Ref<PKResourceMeshEntry> p_entry) {
	Ref<Resource> resource = ResourceLoader::get_singleton()->load(p_path);
	ERR_FAIL_NULL(resource);
	p_entry->gd_resource = resource;
	const Callable callable = callable_mp_static(PKResourceHandlerMesh::_update_mesh).bind(p_entry);
	if (resource->is_connected("changed", callable)) {
		resource->disconnect("changed", callable); // reconnect to update the entry parameter
	}

	// Use deferred connection, popcorn may reload the mesh when told it was changed, causing a ResourceLoader assert to pop up when trying to load the mesh currently being reimported
	resource->connect("changed", callable, Object::CONNECT_DEFERRED);
	_update_mesh(p_entry);
}

} // namespace godot
