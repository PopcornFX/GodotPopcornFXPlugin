//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/projection.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include "pk_sdk.h"

#include <pk_kernel/include/kr_string.h>
#include <pk_maths/include/pk_maths_primitives.h>
#include <pk_particles/include/ps_samplers_image.h>

using namespace godot;

template <typename OutType, typename InType>
_FORCE_INLINE_ const OutType &reinterpret(const InType &p_vec) {
	PK_STATIC_ASSERT(sizeof(InType) == sizeof(OutType));
	return *reinterpret_cast<const OutType *>(&p_vec);
}

_FORCE_INLINE_ Vector2 to_gd(const CFloat2 &p_vec) {
	return reinterpret<Vector2>(p_vec);
}
_FORCE_INLINE_ Vector3 to_gd(const CFloat3 &p_vec) {
	return reinterpret<Vector3>(p_vec);
}
_FORCE_INLINE_ Vector4 to_gd(const CFloat4 &p_vec) {
	return reinterpret<Vector4>(p_vec);
}
_FORCE_INLINE_ Quaternion to_gd(const CQuaternion &p_vec) {
	return reinterpret<Quaternion>(p_vec);
}
_FORCE_INLINE_ AABB to_gd(const CAABB &p_bbox) {
	return AABB(to_gd(p_bbox.Min()), to_gd(p_bbox.Extent()));
}

_FORCE_INLINE_ CFloat2 to_pk(const Vector2 &p_vec) {
	return reinterpret<CFloat2>(p_vec);
}
_FORCE_INLINE_ CFloat3 to_pk(const Vector3 &p_vec) {
	return reinterpret<CFloat3>(p_vec);
}
_FORCE_INLINE_ CFloat4 to_pk(const Vector4 &p_vec) {
	return reinterpret<CFloat4>(p_vec);
}
_FORCE_INLINE_ CFloat4 to_pk(const Color &p_vec) {
	return reinterpret<CFloat4>(p_vec);
}
_FORCE_INLINE_ CQuaternion to_pk(const Quaternion &p_vec) {
	return reinterpret<CQuaternion>(p_vec);
}
_FORCE_INLINE_ CAABB to_pk(const AABB &p_bbox) {
	return reinterpret<CAABB>(p_bbox);
}

_FORCE_INLINE_ CImage::EFormat to_pk(const Image::Format &p_format) {
	switch (p_format) {
		case Image::Format::FORMAT_RGB8: // Needs manual conversion
			return CImage::EFormat::Format_BGR8;
		case Image::Format::FORMAT_RGBA8: // Needs manual conversion
			return CImage::EFormat::Format_BGRA8;
		case Image::Format::FORMAT_L8:
			return CImage::EFormat::Format_Lum8;
		case Image::Format::FORMAT_LA8:
			return CImage::EFormat::Format_LumAlpha8;
		case Image::Format::FORMAT_ETC:
			return CImage::EFormat::Format_RGB8_ETC1;
		case Image::Format::FORMAT_ETC2_RGB8:
			return CImage::EFormat::Format_RGB8_ETC2;
		case Image::Format::FORMAT_ETC2_RGBA8:
			return CImage::EFormat::Format_RGBA8_ETC2;
		case Image::Format::FORMAT_ETC2_RGB8A1:
			return CImage::EFormat::Format_RGB8A1_ETC2;
		case Image::Format::FORMAT_DXT1:
			return CImage::EFormat::Format_DXT1;
		case Image::Format::FORMAT_DXT3:
			return CImage::EFormat::Format_DXT3;
		case Image::Format::FORMAT_DXT5:
			return CImage::EFormat::Format_DXT5;
		case Image::Format::FORMAT_RGBH: // Needs Godot conversion to RGBAH
			return CImage::EFormat::Format_Fp16RGBA;
		case Image::Format::FORMAT_RGBAH:
			return CImage::EFormat::Format_Fp16RGBA;
		case Image::Format::FORMAT_RGBF: // Needs Godot conversion to RGBAF
			return CImage::EFormat::Format_Fp32RGBA;
		case Image::Format::FORMAT_RGBAF:
			return CImage::EFormat::Format_Fp32RGBA;
		default: {
			return CImage::EFormat::Format_Invalid;
		}
	}
}

#define FOREACH_GODOT_IMAGE_FORMAT(op) \
	op(FORMAT_L8);                     \
	op(FORMAT_LA8);                    \
	op(FORMAT_R8);                     \
	op(FORMAT_RG8);                    \
	op(FORMAT_RGB8);                   \
	op(FORMAT_RGBA8);                  \
	op(FORMAT_RGBA4444);               \
	op(FORMAT_RGB565);                 \
	op(FORMAT_RF);                     \
	op(FORMAT_RGF);                    \
	op(FORMAT_RGBF);                   \
	op(FORMAT_RGBAF);                  \
	op(FORMAT_RH);                     \
	op(FORMAT_RGH);                    \
	op(FORMAT_RGBH);                   \
	op(FORMAT_RGBAH);                  \
	op(FORMAT_RGBE9995);               \
	op(FORMAT_DXT1);                   \
	op(FORMAT_DXT3);                   \
	op(FORMAT_DXT5);                   \
	op(FORMAT_RGTC_R);                 \
	op(FORMAT_RGTC_RG);                \
	op(FORMAT_BPTC_RGBA);              \
	op(FORMAT_BPTC_RGBF);              \
	op(FORMAT_BPTC_RGBFU);             \
	op(FORMAT_ETC);                    \
	op(FORMAT_ETC2_R11);               \
	op(FORMAT_ETC2_R11S);              \
	op(FORMAT_ETC2_RG11);              \
	op(FORMAT_ETC2_RG11S);             \
	op(FORMAT_ETC2_RGB8);              \
	op(FORMAT_ETC2_RGBA8);             \
	op(FORMAT_ETC2_RGB8A1);            \
	op(FORMAT_ETC2_RA_AS_RG);          \
	op(FORMAT_DXT5_RA_AS_RG);          \
	op(FORMAT_ASTC_4x4);               \
	op(FORMAT_ASTC_4x4_HDR);           \
	op(FORMAT_ASTC_8x8);               \
	op(FORMAT_ASTC_8x8_HDR);

#define GODOT_IMAGE_FORMAT_ENUM_TO_STRING(format) \
	case Image::Format::format:                   \
		return String(#format)

_FORCE_INLINE_ String godot_image_format_to_string(const Image::Format &p_format) {
	switch (p_format) {
		FOREACH_GODOT_IMAGE_FORMAT(GODOT_IMAGE_FORMAT_ENUM_TO_STRING)
		default:
			return "FORMAT_MAX";
	}
}

_FORCE_INLINE_ CFloat4x4 to_pk(const Transform3D &p_transform) {
	CFloat4x4 out;
	const Basis basis = p_transform.get_basis();
	const Vector3 origin = p_transform.get_origin();

	out.Axis(0) = CFloat4(basis[0][0], basis[1][0], basis[2][0], 0);
	out.Axis(1) = CFloat4(basis[0][1], basis[1][1], basis[2][1], 0);
	out.Axis(2) = CFloat4(basis[0][2], basis[1][2], basis[2][2], 0);
	out.Axis(3) = CFloat4(origin[0], origin[1], origin[2], 1);

	return out;
}

_FORCE_INLINE_ CFloat4x4 to_pk(const Projection &p_projection) {
	CFloat4x4 out;

	out.Axis(0) = CFloat4(p_projection[0][0], p_projection[1][0], p_projection[2][0], p_projection[3][0]);
	out.Axis(1) = CFloat4(p_projection[0][1], p_projection[1][1], p_projection[2][1], p_projection[3][1]);
	out.Axis(2) = CFloat4(p_projection[0][2], p_projection[1][2], p_projection[2][2], p_projection[3][2]);
	out.Axis(3) = CFloat4(p_projection[0][3], p_projection[1][3], p_projection[2][3], p_projection[3][3]);

	return out;
}

// Strings
_FORCE_INLINE_ String to_gd(const CString &p_str) {
	return String(p_str.Data());
}
_FORCE_INLINE_ CString to_pk(const String &p_str) {
	return CString(p_str.utf8().ptr());
}

// This is allocated before PopcornFX startup. Do not use PopcornFX types
struct SPopcornFXProjectSettings {
#if TOOLS_ENABLED
	String source_pack;
	bool debug_baked_effects;
#endif
	String source_pack_root;
};
