//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/classes/texture2d.hpp"

#include "integration/pk_sdk.h"
#include "pk_attribute_sampler.h"

#include <pk_particles/include/ps_samplers_image.h>

namespace godot {

class Texture2D;

class PKAttributeSamplerImage : public PKAttributeSampler {
	GDCLASS(PKAttributeSamplerImage, PKAttributeSampler);

public:
	PKAttributeSamplerImage() = default;
	PKAttributeSamplerImage(const PopcornFX::CResourceDescriptor *);
	~PKAttributeSamplerImage();

	Ref<Texture2D> get_image_resource() const { return image_resource; }
	void set_image_resource(Ref<Texture2D> p_image_resource);

	TResourcePtr<CImage> image;
	CImageSampler *image_sampler = null;

protected:
	static void _bind_methods();
	void _changed();
	void _disconnect_changed();

	void _clear();
	bool _rebuild_image_sampler();

	Ref<Texture2D> image_resource;
	PParticleSamplerDescriptor_Image_Default image_desc;
};

} // namespace godot
