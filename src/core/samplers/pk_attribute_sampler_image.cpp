//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_attribute_sampler_image.h"

#include "godot_cpp/classes/curve.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/texture2d.hpp"

#include "integration/internal/pk_resource_handler_image.h"
#include "integration/pk_plugin_types.h"

namespace godot {

PKAttributeSamplerImage::PKAttributeSamplerImage(const PopcornFX::CResourceDescriptor *) {
}

PKAttributeSamplerImage::~PKAttributeSamplerImage() {
	_clear();
}

void PKAttributeSamplerImage::set_image_resource(Ref<Texture2D> p_image_resource) {
	if (p_image_resource.is_valid() && !p_image_resource->is_class(Texture2D::get_class_static())) {
		ERR_FAIL_MSG(vformat("Cannot use %s as a image to sample.", p_image_resource->get_class()));
	}

	if (image_resource.is_valid()) {
		image_resource->disconnect("changed", callable_mp(this, &PKAttributeSamplerImage::_changed));
	}

	image_resource = p_image_resource;
	if (image_resource.is_valid()) {
		image_resource->connect("changed", callable_mp(this, &PKAttributeSamplerImage::_changed));
		_changed();
	} else {
		_clear();
		emit_changed();
	}
}

void PKAttributeSamplerImage::_bind_methods() {
	BIND_BASIC_PROPERTY(PKAttributeSamplerImage, OBJECT, image_resource, PROPERTY_HINT_RESOURCE_TYPE, "Texture2D", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_ALWAYS_DUPLICATE);
	ClassDB::bind_method(D_METHOD("changed"), &PKAttributeSamplerImage::_changed);
}

void PKAttributeSamplerImage::_changed() {
	if (!_rebuild_image_sampler()) {
		_clear();
	} else {
		desc = image_desc.Get();
	}
	emit_changed();
}

void PKAttributeSamplerImage::_disconnect_changed() {
	if (image_resource.is_valid()) {
		image_resource->disconnect("changed", callable_mp(this, &PKAttributeSamplerImage::_changed));
	}
}

void PKAttributeSamplerImage::_clear() {
	PK_SAFE_DELETE(image_sampler);
	if (image != nullptr) {
		image.Clear();
	}
	desc = nullptr;
}

bool PKAttributeSamplerImage::_rebuild_image_sampler() {
	if (!image_resource->is_class(Texture2D::get_class_static())) {
		ERR_FAIL_V_MSG(false, vformat("Invalid texture class for image attribute sampler '%s'", get_name()));
	}

	Ref<Image> img;
	const RID rid = image_resource->get_rid();
	if (rid.is_valid()) {
		const RenderingServer *rs = RenderingServer::get_singleton();
		img = rs->texture_2d_get(rid);
	}

	CImageSurface dst_surface;
	if (img.is_valid() && !img->is_empty()) { // Pixels are instantly reachable, can set them to a CImageSurface
		dst_surface = PKResourceHandlerImage::new_surface_from_gd_img(img);
	} else { // Didn't find the image, must use the Resource handler to retrieve it
		image = PopcornFX::Resource::DefaultManager()->Load<CImage>(to_pk(image_resource->get_path()), true);
		if (image == nullptr || image->Empty() || image->m_Frames.Empty() || image->m_Frames[0].m_Mipmaps.Empty()) {
			ERR_FAIL_V_MSG(false, vformat("Could not create CImage from image path %s", image_resource->get_path()));
		}
		dst_surface = CImageSurface(image->m_Frames[0].m_Mipmaps[0], image->m_Format);
	}

	if (image_sampler == nullptr) {
		image_sampler = PK_NEW(CImageSamplerBilinear);
		if (image_sampler == nullptr) {
			ERR_FAIL_V_MSG(false, vformat("Could not create CImageSampler from image '%s'", image_resource->get_path()));
		}
	}

	if (!image_sampler->SetupFromSurface(dst_surface)) {
		CImageSurface converted_surface;
		converted_surface.m_Format = CImage::Format_BGRA8;
		UtilityFunctions::print(vformat("Converting image '%s' format '%s' to '%s'",
				image_resource->get_path(), CImage::GetFormatName(dst_surface.m_Format), CImage::GetFormatName(converted_surface.m_Format)));
		if (!converted_surface.CopyAndConvertIFN(dst_surface)) {
			ERR_FAIL_V_MSG(false, vformat("Could not convert image '%s' to '%s'", image_resource->get_path(), CImage::GetFormatName(converted_surface.m_Format)));
		}
		if (!image_sampler->SetupFromSurface(converted_surface)) {
			ERR_FAIL_V_MSG(false, vformat("Could not create CImageSampler from image '%s' even after converting it to 'BRGA8'", image_resource->get_path()));
		}
	}

	if (image_desc == nullptr) {
		image_desc = PK_NEW(CParticleSamplerDescriptor_Image_Default());
		if (image_desc == nullptr) {
			ERR_FAIL_V_MSG(false, vformat("Could not create CParticleSamplerDescriptor_Image_Default"));
		}
	}

	image_desc->m_Sampler = image_sampler;
	return true;
}

} // namespace godot
