//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_sdk.h"

#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/packed_scene.hpp"
#include "godot_cpp/classes/ref.hpp"

#include <pk_geometrics/include/ge_mesh_resource.h>
#include <pk_kernel/include/kr_resources.h>

namespace godot {

class Image;
class Resource;

struct RawSurfaceData {
	String name;
	PackedVector3Array vertices;
	PackedInt32Array indices;
	PackedVector3Array normals;
	PackedFloat32Array tangents;
	PackedColorArray colors;
	PackedVector2Array UVs;
	PackedVector2Array UV2s;
};

struct RawMeshData {
	String name;
	Vector<RawSurfaceData> surfaces;
};

class PKResourceMeshEntry : public RefCounted {
	GDCLASS(PKResourceMeshEntry, RefCounted)
public:
	~PKResourceMeshEntry();
	static void _bind_methods() {}
	PResourceMesh pk_resource;
	Ref<Resource> gd_resource;
};

class PKResourceHandlerMesh : public IResourceHandler {
public:
	static void startup();
	static void shutdown();

	PKResourceHandlerMesh() = default;
	~PKResourceHandlerMesh() = default;

	static CResourceMesh *new_from_gd_meshes(const Vector<Pair<Ref<Mesh>, Transform3D>> &p_meshes);
	static CResourceMesh *new_from_gd_resource(const Ref<Resource> p_resource);

	static Ref<ArrayMesh> gd_merge_packed_scene_mesh(const Ref<PackedScene> p_scene);

	virtual void *Load(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id, // used to check we are called with the correct type
			const CString &p_resource_path,
			bool p_path_not_virtual,
			const SResourceLoadCtl &p_load_ctl,
			CMessageStream &p_load_report,
			SAsyncLoadStatus *p_async_load_status) override; // if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress

	virtual void *Load(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id, // used to check we are called with the correct type
			const CFilePackPath &p_resource_path,
			const SResourceLoadCtl &p_load_ctl,
			CMessageStream &p_load_report,
			SAsyncLoadStatus *p_async_load_status) override; // if null, will be a synchronous load, if not, the async loader will update the structure to tell the progress

	virtual void Unload(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id, // used to check we are called with the correct type
			void *p_resource) override;

	virtual void AppendDependencies(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id,
			void *p_resource,
			TArray<CString> &r_resource_paths) const override;

	virtual void AppendDependencies(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id,
			const CString &p_resource_path,
			bool p_path_not_virtual,
			TArray<CString> &r_resource_paths) const override;

	virtual void AppendDependencies(
			const CResourceManager *p_resource_manager,
			u32 p_resource_type_id,
			const CFilePackPath &p_resource_path,
			TArray<CString> &r_resource_paths) const override;

private:
	static Vector<RawMeshData> _raw_from_gd_meshes(const Vector<Pair<Ref<Mesh>, Transform3D>> &p_meshes);
	static Vector<Pair<Ref<Mesh>, Transform3D>> _meshes_from_packed_scene(const Ref<PackedScene> p_packed_scene);

	static void _update_mesh(Ref<PKResourceMeshEntry> p_entry);
	static void _deferred_update_mesh(const String& p_path, Ref<PKResourceMeshEntry> p_entry);
};

} // namespace godot
