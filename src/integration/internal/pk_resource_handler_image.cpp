//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_resource_handler_image.h"

#include "godot_cpp/classes/atlas_texture.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/classes/noise_texture2d.hpp"
#include "godot_cpp/classes/texture2d.hpp"

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/resource_loader.hpp"

#include "integration/engine/pk_file_system_controller.h"
#include "integration/pk_error_handling.h"
#include "integration/pk_plugin_types.h"

#include <pk_kernel/include/kr_refcounted_buffer.h>

namespace godot {
namespace {
PKResourceHandlerImage *g_resource_handler_image_godot = nullptr;
} //namespace

void PKResourceHandlerImage::startup() {
	PKGD_ASSERT(g_resource_handler_image_godot == nullptr);
	g_resource_handler_image_godot = PK_NEW(PKResourceHandlerImage);
	if (!PK_VERIFY(g_resource_handler_image_godot != nullptr)) {
		return;
	}
	PopcornFX::Resource::DefaultManager()->RegisterHandler<CImage>(g_resource_handler_image_godot);
}

void PKResourceHandlerImage::shutdown() {
	if (PK_VERIFY(g_resource_handler_image_godot != nullptr)) {
		PopcornFX::Resource::DefaultManager()->UnregisterHandler<PopcornFX::CImage>(g_resource_handler_image_godot);
		PK_DELETE(g_resource_handler_image_godot);
		g_resource_handler_image_godot = nullptr;
	}
}

CImageSurface PKResourceHandlerImage::new_surface_from_gd_img(const Ref<Image> p_image) {
	CImage::EFormat pk_src_format = to_pk(p_image->get_format());
	if (pk_src_format == CImage::EFormat::Format_Invalid) {
		ERR_FAIL_V_MSG(CImageSurface(), vformat("Can't sample image '%s', image format '%s' is not supported", p_image->get_name(), godot_image_format_to_string(p_image->get_format())));
	}

	return CImageSurface(CUint3(static_cast<u32>(p_image->get_width()), static_cast<u32>(p_image->get_height()), 1u),
			pixel_buff_from_gd_img(p_image), pk_src_format);
}

CImage *PKResourceHandlerImage::new_from_gd_img(const Ref<Image> p_image) {
	CImage::EFormat pk_src_format = to_pk(p_image->get_format());
	if (pk_src_format == CImage::EFormat::Format_Invalid) {
		ERR_FAIL_V_MSG(nullptr, vformat("Can't sample image '%s', image format '%s' is not supported", p_image->get_name(), godot_image_format_to_string(p_image->get_format())));
	}

	CImage *new_image = PK_NEW(CImage);
	if (!PK_VERIFY(new_image != nullptr) ||
		!PK_VERIFY(new_image->m_Frames.Resize(1)) ||
		!PK_VERIFY(new_image->m_Frames[0].m_Mipmaps.Resize(1))) {
		PK_DELETE(new_image);
		return nullptr;
	}
	new_image->m_Format = pk_src_format;
	new_image->m_Frames[0].m_Mipmaps[0].m_RawBuffer = pixel_buff_from_gd_img(p_image);
	new_image->m_Frames[0].m_Mipmaps[0].m_Dimensions = CUint3(static_cast<u32>(p_image->get_width()), static_cast<u32>(p_image->get_height()), 1u);
	return new_image;
}

CImage *PKResourceHandlerImage::new_from_gd_resource(const Ref<Resource> p_image_resource) {
	if (!p_image_resource->is_class(Texture2D::get_class_static())) {
		return nullptr;
	}

	Ref<Image> img;
	if (p_image_resource->is_class(AtlasTexture::get_class_static())) {
		const Ref<AtlasTexture> atlas = p_image_resource;
		img = atlas->get_image(); // get_image() is overriden for this class
	} else if (p_image_resource->is_class(NoiseTexture2D::get_class_static())) {
		const Ref<NoiseTexture2D> noise = p_image_resource;
		img = noise->get_image(); // get_image() is overriden for this class
	} else {
		const Ref<Texture2D> texture = p_image_resource;
		img = RenderingServer::get_singleton()->texture_2d_get(texture->get_rid());
	}

	if (!img.is_valid()) {
		ERR_FAIL_V_MSG(nullptr, vformat("Unsupported resource type '%s' for creation of PopcornFX image resource", p_image_resource->get_path()));
	}

	return new_from_gd_img(img);
}

void PKResourceHandlerImage::convert_rgb8_to_bgr8(void *r_dst, const void *p_src, u32 p_size) {
	uint8_t *dst_data = static_cast<uint8_t *>(r_dst);
	const uint8_t *src_data = static_cast<const uint8_t *>(p_src);
	for (uint32_t i = 0; i < p_size; i += 3) {
		dst_data[i + 2] = src_data[i];
		dst_data[i + 1] = src_data[i + 1];
		dst_data[i] = src_data[i + 2];
	}
}

void PKResourceHandlerImage::convert_rgba8_to_bgra8(void *r_dst, const void *p_src, u32 p_size) {
	uint8_t *dst_data = static_cast<uint8_t *>(r_dst);
	const uint8_t *src_data = static_cast<const uint8_t *>(p_src);
	for (uint32_t i = 0; i < p_size; i += 4) {
		dst_data[i + 2] = src_data[i];
		dst_data[i + 1] = src_data[i + 1];
		dst_data[i] = src_data[i + 2];
		dst_data[i + 3] = src_data[i + 3];
	}
}

PRefCountedMemoryBuffer PKResourceHandlerImage::pixel_buff_from_gd_img(const Ref<Image> p_image) {
	if (p_image->get_format() == Image::Format::FORMAT_RGBF) {
		p_image->convert(Image::Format::FORMAT_RGBAF);
	}
	if (p_image->get_format() == Image::Format::FORMAT_RGBH) {
		p_image->convert(Image::Format::FORMAT_RGBAH);
	}
	PRefCountedMemoryBuffer dst_buffer = CRefCountedMemoryBuffer::AllocAligned(p_image->get_data_size() + 0x80, 0x80);

	switch (p_image->get_format()) {
		case Image::Format::FORMAT_RGB8: {
			convert_rgb8_to_bgr8(dst_buffer.Get()->Data<void>(), p_image->ptr(), p_image->get_data_size());
			break;
		}
		case Image::Format::FORMAT_RGBA8: {
			convert_rgba8_to_bgra8(dst_buffer.Get()->Data<void>(), p_image->ptr(), p_image->get_data_size());
			break;
		}
		default: {
			memcpy(dst_buffer.Get()->Data<void>(), p_image->ptr(), p_image->get_data_size());
			break;
		}
	}

	return dst_buffer;
}

void *PKResourceHandlerImage::Load(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CString &p_resource_path,
		bool p_path_not_virtual,
		const SResourceLoadCtl &p_load_ctl,
		CMessageStream &p_load_report,
		SAsyncLoadStatus *p_async_load_status) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());

	PKFileSystemController *file_system = static_cast<PKFileSystemController *>(p_resource_manager->FileController());
	String godot_path = file_system->sanitize_path(to_gd(p_resource_path), !p_path_not_virtual);

	if (!PKGD_VERIFY(!godot_path.begins_with("::"))) {
		ERR_FAIL_V_MSG(nullptr, vformat("Can't load temporary texture resources. ('%s')", godot_path));
	}

	if (godot_path.is_empty()) {
		if (p_async_load_status != nullptr) {
			p_async_load_status->m_Resource = nullptr;
			p_async_load_status->m_Done = true;
			p_async_load_status->m_Progress = 1.0f;
		}
		return nullptr;
	}

	ResourceLoader *rl = ResourceLoader::get_singleton();
	const Ref<Texture2D> texture_resource = rl->load(godot_path, String("Texture2D"), ResourceLoader::CACHE_MODE_REUSE);
	if (!texture_resource.is_valid()) {
		if (p_async_load_status != nullptr) {
			p_async_load_status->m_Resource = nullptr;
			p_async_load_status->m_Done = true;
			p_async_load_status->m_Progress = 1.0f;
		}
		ERR_FAIL_V_MSG(nullptr, vformat("Can't load texture resource '%s'", godot_path));
	}

	CImage *resource = new_from_gd_resource(texture_resource);

	if (p_async_load_status != nullptr) {
		p_async_load_status->m_Resource = resource;
		p_async_load_status->m_Done = true;
		p_async_load_status->m_Progress = 1.0f;
	}
	return resource;
}

void *PKResourceHandlerImage::Load(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CFilePackPath &p_resource_path,
		const SResourceLoadCtl &p_load_ctl,
		CMessageStream &p_load_report,
		SAsyncLoadStatus *p_async_load_status) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());
	return Load(p_resource_manager, p_resource_type_id, p_resource_path.Path(), false, p_load_ctl, p_load_report, p_async_load_status);
}

void PKResourceHandlerImage::Unload(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		void *p_resource) {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());
}

void PKResourceHandlerImage::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		void *p_resource,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());
}

void PKResourceHandlerImage::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CString &p_resource_path,
		bool p_path_not_virtual,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());
}

void PKResourceHandlerImage::AppendDependencies(
		const CResourceManager *p_resource_manager,
		u32 p_resource_type_id,
		const CFilePackPath &p_resource_path,
		TArray<CString> &r_resource_paths) const {
	PKGD_ASSERT(p_resource_type_id == TResourceRouter<CImage>::ResourceTypeID());
}
} // namespace godot
