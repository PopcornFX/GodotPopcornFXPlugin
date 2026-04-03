//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/ref.hpp"

#include "integration/pk_sdk.h"

#include <pk_kernel/include/kr_resources.h>
#include <pk_particles/include/ps_samplers_image.h>

namespace godot {

class Image;
class Resource;

class PKResourceHandlerImage : public IResourceHandler {
public:
	static void startup();
	static void shutdown();

	PKResourceHandlerImage() = default;
	~PKResourceHandlerImage() = default;

	/* Creates a CImageSurface from a Godot Image */
	static CImageSurface new_surface_from_gd_img(const Ref<Image> p_image);
	/* Creates a CImage from a Godot Image */
	static CImage *new_from_gd_img(const Ref<Image> p_image);
	/* Creates a CImage from a Godot Resource if possible */
	static CImage *new_from_gd_resource(const Ref<Resource> p_image_resource);
	/* Naively converts a RGB source pixel pointer to BRG */
	static void convert_rgb8_to_bgr8(void *r_dst, const void *p_src, u32 p_size);
	/* Naively converts a RGB source pixel pointer to BRGA */
	static void convert_rgba8_to_bgra8(void *r_dst, const void *p_src, u32 p_size);

	/*
		If the format is supported, creates an aligned PopcornFX buffer and copies Godot image data into it.
		Converts some formats if necessary
	*/
	static PRefCountedMemoryBuffer pixel_buff_from_gd_img(const Ref<Image> p_image);

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
};

} // namespace godot
