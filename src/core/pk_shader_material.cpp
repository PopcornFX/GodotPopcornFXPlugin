//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_shader_material.h"

#include "godot_cpp/classes/fbx_document.hpp"
#include "godot_cpp/classes/fbx_state.hpp"
#include "godot_cpp/classes/gltf_mesh.hpp"
#include "godot_cpp/classes/importer_mesh.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/classes/shader_material.hpp"

#include "integration/engine/render/pk_renderer_cache.h"
#include "integration/pk_plugin_types.h"

#include <pk_particles/include/Renderers/ps_renderer_base.h>
#include <pk_render_helpers/include/render_features/rh_features_basic.h>

using RS = RenderingServer;

static Ref<Texture2D> load_texture_pk(const CString &p_pk_path) {
	if (p_pk_path.Empty()) {
		return nullptr;
	}

	ResourceLoader *loader = ResourceLoader::get_singleton();
	String path = p_pk_path.Data();
	path = File::DefaultFileSystem()->VirtualToPhysical(p_pk_path, IFileSystem::Access_Read).Data();
	return loader->load(path, "Texture2D");
}

static Ref<Mesh> load_mesh_pk(const CString &p_pk_path) {
	if (p_pk_path.Empty()) {
		return nullptr;
	}

	String path = p_pk_path.Data();
	path = File::DefaultFileSystem()->VirtualToPhysical(p_pk_path, IFileSystem::Access_Read).Data();

	Ref<FBXDocument> document;
	Ref<FBXState> state;
	document.instantiate();
	state.instantiate();

	const Error err = document->append_from_file(path, state);
	if (err != OK) {
		return nullptr;
	}

	const TypedArray<Ref<GLTFMesh>> gltf_meshes = state->get_meshes();
	if (gltf_meshes.is_empty()) {
		return nullptr;
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	RS *rs = RS::get_singleton();
	const RID mesh_rid = mesh->get_rid();

	// Copy surfaces from all meshes in the file
	for (const Ref<GLTFMesh> gltf_mesh : gltf_meshes) {
		const Ref<ImporterMesh> importer_mesh = gltf_mesh->get_mesh();

		const uint32_t surface_count = importer_mesh->get_surface_count();
		for (uint32_t j = 0; j < surface_count; ++j) {
			mesh->add_surface_from_arrays(importer_mesh->get_surface_primitive_type(j), importer_mesh->get_surface_arrays(j));
		}
	}
	return mesh;
}

String PKShaderMaterial::get_blend_mode_name(BlendMode p_mode) {
	switch (p_mode) {
		case BlendMode::OPAQUE:
			return "OPAQUE";
		case BlendMode::MASKED:
			return "MASKED";
		case BlendMode::ADDITIVE:
			return "ADDITIVE";
		case BlendMode::ADDITIVE_NO_ALPHA:
			return "ADDITIVE_NO_ALPHA";
		case BlendMode::ALPHA_BLEND:
			return "ALPHA";
		case BlendMode::PREMUL_ALPHA:
			return "PREMUL_ALPHA";
		default:
			return "";
	}
}

String PKShaderMaterial::get_variation_flag_name(VariationFlags p_flag) {
	switch (p_flag) {
		case VariationFlags::LIGHTING:
			return "LIGHTING";
		case VariationFlags::LIGHTING_LEGACY:
			return "LIGHTING_LEGACY";
		case VariationFlags::DOUBLE_SIDED:
			return "DOUBLE_SIDED";
		case VariationFlags::DIFFUSE:
			return "DIFFUSE";
		case VariationFlags::DIFFUSE_RAMP:
			return "DIFFUSE_RAMP";
		case VariationFlags::EMISSIVE:
			return "EMISSIVE";
		case VariationFlags::EMISSIVE_RAMP:
			return "EMISSIVE_RAMP";
		case VariationFlags::ALPHA_REMAP:
			return "ALPHA_REMAP";
		case VariationFlags::ALPHA_MASKS:
			return "ALPHA_MASKS";
		case VariationFlags::SIZE_2D:
			return "SIZE2D";
		case VariationFlags::ATLAS:
			return "ATLAS";
		case VariationFlags::ATLAS_BLEND_SOFT:
			return "ATLAS_BLEND_SOFT";
		case VariationFlags::ATLAS_BLEND_MV:
			return "ATLAS_BLEND_MV";
		case VariationFlags::TRANSFORM_UVS:
			return "TRANSFORM_UVS";
		case VariationFlags::SOFT_PARTICLES:
			return "SOFT_PARTICLES";
		case VariationFlags::CORRECT_DEFORMATION:
			return "CORRECT_DEFORMATION";
		case VariationFlags::DISTORTION:
			return "DISTORTION";
		case VariationFlags::UV_DISTORTIONS:
			return "UV_DISTORTIONS";
		case VariationFlags::DISSOLVE:
			return "DISSOLVE";
		case VariationFlags::VAT_FLUID:
			return "VAT_FLUID";
		case VariationFlags::VAT_SOFT:
			return "VAT_SOFT";
		case VariationFlags::VAT_RIGID:
			return "VAT_RIGID";
		case VariationFlags::SKELETAL_ANIM:
			return "SKELETAL_ANIM";
		case VariationFlags::SKELETAL_INTERPOL:
			return "SKELETAL_INTERPOL";
		case VariationFlags::SKELETAL_TRACK_INTERPOL:
			return "SKELETAL_TRACK_INTERPOL";
		default:
			return "";
	}
}

void PKShaderMaterial::init_from_renderer(const CRendererDataBase &p_renderer) {
	const SRendererDeclaration &decl = p_renderer.m_Declaration;
	renderer_type = p_renderer.m_RendererType;
	variation_flags = VariationFlags::NONE;
	cast_shadows = false;

	const bool legacy_lit_opaque = decl.IsFeatureEnabled(BasicRendererProperties::SID_LegacyLitOpaque());
	const bool legacy_lit = decl.IsFeatureEnabled(BasicRendererProperties::SID_LegacyLit());
	const bool lit = decl.IsFeatureEnabled(BasicRendererProperties::SID_Lit());

	// Lighting
	if (legacy_lit_opaque) {
		set_variation_flag(VariationFlags::LIGHTING_LEGACY);
		cast_shadows |= decl.GetPropertyValue_B(BasicRendererProperties::SID_LegacyLitOpaque_CastShadows(), false);
		tex_normal = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_LegacyLitOpaque_NormalMap(), CString::EmptyString));
		tex_specular = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_LegacyLitOpaque_SpecularMap(), CString::EmptyString));
	} else if (legacy_lit) {
		set_variation_flag(VariationFlags::LIGHTING_LEGACY);
		cast_shadows |= decl.GetPropertyValue_B(BasicRendererProperties::SID_LegacyLit_CastShadows(), false);
		tex_normal = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_LegacyLit_NormalMap(), CString::EmptyString));
		tex_specular = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_LegacyLit_SpecularMap(), CString::EmptyString));
	} else if (lit) {
		set_variation_flag(VariationFlags::LIGHTING);
		cast_shadows |= decl.GetPropertyValue_B(BasicRendererProperties::SID_Lit_CastShadows(), false);
		tex_normal = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Lit_NormalMap(), CString::EmptyString));
		tex_rough_metal = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Lit_RoughMetalMap(), CString::EmptyString));
		roughness = decl.GetPropertyValue_F1(BasicRendererProperties::SID_Lit_Roughness(), 1.0f);
		metalness = decl.GetPropertyValue_F1(BasicRendererProperties::SID_Lit_Metalness(), 0.0f);
	}

	// Transparency / Opaque
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Transparent())) {
		const int transparent_type = decl.GetPropertyValue_I1(BasicRendererProperties::SID_Transparent_Type(), 0);
		switch (transparent_type) {
			case BasicRendererProperties::Additive:
				blend_mode = BlendMode::ADDITIVE;
				break;
			case BasicRendererProperties::AdditiveNoAlpha:
				blend_mode = BlendMode::ADDITIVE_NO_ALPHA;
				break;
			case BasicRendererProperties::PremultipliedAlpha:
				blend_mode = BlendMode::PREMUL_ALPHA;
				break;
			default:
				blend_mode = BlendMode::ALPHA_BLEND;
				break;
		}
		transparent_sort_override = decl.GetPropertyValue_I1(BasicRendererProperties::SID_Transparent_GlobalSortOverride(), 0);
		transparent_sort_offset = decl.GetPropertyValue_F1(BasicRendererProperties::SID_Transparent_CameraSortOffset(), 0);
	} else if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Opaque())) {
		const int opaque_type = decl.GetPropertyValue_I1(BasicRendererProperties::SID_Opaque_Type(), 0);
		if (opaque_type == BasicRendererProperties::Masked) {
			blend_mode = BlendMode::MASKED;
		} else {
			blend_mode = BlendMode::OPAQUE;
		}
		cast_shadows |= decl.GetPropertyValue_B(BasicRendererProperties::SID_Opaque_CastShadows(), false);
		mask_threshold = decl.GetPropertyValue_F1(BasicRendererProperties::SID_Opaque_MaskThreshold(), 0.0f);
	}

	// Diffuse
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Diffuse())) {
		set_variation_flag(VariationFlags::DIFFUSE);
		tex_diffuse = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Diffuse_DiffuseMap(), CString::EmptyString));
		if (decl.IsFeatureEnabled(BasicRendererProperties::SID_DiffuseRamp())) {
			set_variation_flag(VariationFlags::DIFFUSE_RAMP);
			tex_diffuse_ramp = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_DiffuseRamp_RampMap(), CString::EmptyString));
		}
	}

	// Distortion
	else if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Distortion())) {
		set_variation_flag(VariationFlags::DISTORTION);
		tex_diffuse = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Distortion_DistortionMap(), CString::EmptyString));
		// TODO: Warn user if texture isn't loaded as a normal map
	}

	// Emissive
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Emissive())) {
		set_variation_flag(VariationFlags::EMISSIVE);
		tex_emissive = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Emissive_EmissiveMap(), CString::EmptyString));
		if (decl.IsFeatureEnabled(BasicRendererProperties::SID_EmissiveRamp())) {
			set_variation_flag(VariationFlags::EMISSIVE_RAMP);
			tex_emissive_ramp = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_EmissiveRamp_RampMap(), CString::EmptyString));
		}
	}

	// Texture atlas
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_Atlas())) {
		set_variation_flag(VariationFlags::ATLAS);
		// TODO: Create a buffer to store atlas values
		//tex_atlas = LoadAtlasPk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Atlas_Definition(), CString::EmptyString));

		const int blending_type = decl.GetPropertyValue_I1(BasicRendererProperties::SID_Atlas_Blending(), 0);
		if (blending_type == BasicRendererProperties::Linear) {
			set_variation_flag(VariationFlags::ATLAS_BLEND_SOFT);
		} else if (blending_type == BasicRendererProperties::MotionVectors) {
			set_variation_flag(VariationFlags::ATLAS_BLEND_MV);
			tex_motion_vectors = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Atlas_MotionVectorsMap(), CString::EmptyString));

			const CFloat2 distortion_strength = decl.GetPropertyValue_F2(BasicRendererProperties::SID_Atlas_DistortionStrength(), CFloat2::ZERO);
			mv_distortion_strength_columns = distortion_strength.x();
			mv_distortion_strength_rows = distortion_strength.y();
		}
	}

	// Transform UVs
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_TransformUVs())) {
		set_variation_flag(VariationFlags::TRANSFORM_UVS);
		transform_uvs_rgb_only = decl.GetPropertyValue_B(BasicRendererProperties::SID_TransformUVs_RGBOnly(), false);
	}

	// Alpha remap
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_AlphaRemap())) {
		set_variation_flag(VariationFlags::ALPHA_REMAP);
		tex_alpha_remap = load_texture_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_AlphaRemap_AlphaMap(), CString::EmptyString));
	}

	// Soft particles
	if (decl.IsFeatureEnabled(BasicRendererProperties::SID_SoftParticles())) {
		set_variation_flag(VariationFlags::SOFT_PARTICLES);
		softness_distance = decl.GetPropertyValue_F1(BasicRendererProperties::SID_SoftParticles_SoftnessDistance(), 0.0f);
	}

	// Mesh
	if (renderer_type == Renderer_Mesh) {
		mesh = load_mesh_pk(decl.GetPropertyValue_Path(BasicRendererProperties::SID_Mesh(), CString::EmptyString));
	}
}

void PKShaderMaterial::create_material_instance() {
	// Load the material and create a new instance
	Ref<Shader> shader;
	{
		ResourceLoader *loader = ResourceLoader::get_singleton();
		const Ref<Shader> loaded_shader = loader->load(_get_shader_path(), "Shader");
		if (loaded_shader.is_valid()) {
			shader = loaded_shader->duplicate_deep();
		}
	}

	// Nothing to do if the shader isn't loaded or there are no variations
	if (shader.is_null() || variation_flags == VariationFlags::NONE) {
		return;
	}

	// Add all shader defines to a string
	String shader_defines = "#define PK_BLEND_" + get_blend_mode_name(blend_mode) + "\n";
	for (uint32_t i = 0; i < (uint32_t)VariationFlags::MAX; ++i) {
		const VariationFlags cur_flag = (VariationFlags)(1 << i);
		if (!has_variation_flag(cur_flag)) {
			continue;
		}

		String flag_name = get_variation_flag_name(cur_flag);
		if (flag_name == "") {
			continue;
		}

		shader_defines += "#define PK_HAS_" + flag_name + "\n";
	}

	// Add constant defines at the start of the material's shader code
	String shader_code = shader->get_code();
	shader_code = shader_code.insert(0, shader_defines);
	shader->set_path("res://addons/popcornfx/assets/materials/"); // Shader uses asset path as include dir (GDExtension doesn't expose Shader::set_include_path)
	shader->set_code(shader_code);
	shader->set_path(""); // Don't keep include path as asset path

	// Create the material instance and set its shader
	material.instantiate();
	material->set_shader(shader);

	// If a mesh has been loaded, set its material for every surface
	if (mesh.is_valid()) {
		const uint32_t surface_count = mesh->get_surface_count();
		for (uint32_t i = 0; i < surface_count; ++i) {
			mesh->surface_set_material(i, material);
		}
	}
}

void PKShaderMaterial::apply_to_visual_instance(RID p_instance) const {
	RS *rs = RS::get_singleton();

	rs->instance_geometry_set_cast_shadows_setting(p_instance,
			cast_shadows ? RS::SHADOW_CASTING_SETTING_ON : RS::SHADOW_CASTING_SETTING_OFF);

	if (is_transparent()) {
		rs->instance_set_pivot_data(p_instance, transparent_sort_offset, true);
	}
}

void PKShaderMaterial::bind_material_properties(const PKRendererCache *p_renderer_cache) const {
	if (is_transparent()) {
		material->set_render_priority(CLAMP(transparent_sort_override, -128, 127));
	}
	if (blend_mode == BlendMode::MASKED) {
		material->set_shader_parameter("in_mask_threshold", mask_threshold);
	}
	if (needs_atlas_rects()) {
		material->set_shader_parameter("in_tex_atlas_rects", p_renderer_cache->get_atlas_rects_srv());
	}
	if (has_variation_flag(VariationFlags::DIFFUSE)) {
		material->set_shader_parameter("in_tex_diffuse", tex_diffuse);
	}
	if (has_variation_flag(VariationFlags::DIFFUSE_RAMP)) {
		material->set_shader_parameter("in_tex_diffuse_ramp", tex_diffuse_ramp);
	}
	if (has_variation_flag(VariationFlags::EMISSIVE)) {
		material->set_shader_parameter("in_tex_emissive", tex_emissive);
	}
	if (has_variation_flag(VariationFlags::EMISSIVE_RAMP)) {
		material->set_shader_parameter("in_tex_emissive_ramp", tex_emissive_ramp);
	}
	if (has_variation_flag(VariationFlags::ALPHA_REMAP)) {
		material->set_shader_parameter("in_tex_alpha_remap", tex_alpha_remap);
	}
	if (has_variation_flag(VariationFlags::LIGHTING)) {
		material->set_shader_parameter("in_roughness", roughness);
		material->set_shader_parameter("in_metalness", metalness);
		material->set_shader_parameter("in_tex_normal", tex_normal);
		material->set_shader_parameter("in_tex_rough_metal", tex_rough_metal);
	}
	if (has_variation_flag(VariationFlags::LIGHTING_LEGACY)) {
		material->set_shader_parameter("in_tex_normal", tex_normal);
		material->set_shader_parameter("in_tex_specular", tex_specular);
	}
	if (has_variation_flag(VariationFlags::ATLAS_BLEND_MV)) {
		material->set_shader_parameter("in_atlas_mv_strength", Vector2(mv_distortion_strength_columns, mv_distortion_strength_rows));
		material->set_shader_parameter("in_tex_atlas_mv", tex_motion_vectors);
	}
	if (has_variation_flag(VariationFlags::TRANSFORM_UVS)) {
		material->set_shader_parameter("in_transform_uvs_rgb_only", transform_uvs_rgb_only);
	}
	if (has_variation_flag(VariationFlags::SOFT_PARTICLES)) {
		material->set_shader_parameter("in_softness_distance", softness_distance);
	}
}

bool PKShaderMaterial::operator==(const PKShaderMaterial &p_other) const {
	return renderer_type == p_other.renderer_type &&
			blend_mode == p_other.blend_mode &&
			variation_flags == p_other.variation_flags &&
			transparent_sort_override == p_other.transparent_sort_override &&
			transparent_sort_offset == p_other.transparent_sort_offset &&
			mask_threshold == p_other.mask_threshold &&
			roughness == p_other.roughness &&
			metalness == p_other.metalness &&
			atlas_sub_div_x == p_other.atlas_sub_div_x &&
			atlas_sub_div_y == p_other.atlas_sub_div_y &&
			mv_distortion_strength_columns == p_other.mv_distortion_strength_columns &&
			mv_distortion_strength_rows == p_other.mv_distortion_strength_rows &&
			softness_distance == p_other.softness_distance &&
			transform_uvs_rgb_only == p_other.transform_uvs_rgb_only &&
			cast_shadows == p_other.cast_shadows &&
			tex_diffuse == p_other.tex_diffuse &&
			tex_diffuse_ramp == p_other.tex_diffuse_ramp &&
			tex_emissive == p_other.tex_emissive &&
			tex_emissive_ramp == p_other.tex_emissive_ramp &&
			tex_normal == p_other.tex_normal &&
			tex_rough_metal == p_other.tex_rough_metal &&
			tex_specular == p_other.tex_specular &&
			tex_alpha_remap == p_other.tex_alpha_remap &&
			tex_motion_vectors == p_other.tex_motion_vectors &&
			mesh == p_other.mesh;
}

bool PKShaderMaterial::is_transparent() const {
	return blend_mode != BlendMode::OPAQUE &&
			blend_mode != BlendMode::MASKED;
}

bool PKShaderMaterial::needs_depth_sorting() const {
	return blend_mode == BlendMode::ALPHA_BLEND ||
			blend_mode == BlendMode::PREMUL_ALPHA;
}

bool PKShaderMaterial::has_lighting() const {
	return has_variation_flag(VariationFlags::LIGHTING) ||
			has_variation_flag(VariationFlags::LIGHTING_LEGACY);
}

bool PKShaderMaterial::has_atlas_blending() const {
	return has_variation_flag(VariationFlags::ATLAS_BLEND_SOFT) ||
			has_variation_flag(VariationFlags::ATLAS_BLEND_MV);
}

bool PKShaderMaterial::needs_atlas_rects() const {
	return has_variation_flag(VariationFlags::ATLAS) &&
			(renderer_type == Renderer_Mesh || has_variation_flag(VariationFlags::TRANSFORM_UVS));
}

bool PKShaderMaterial::has_variation_flag(VariationFlags p_flag) const {
	return ((uint32_t)variation_flags & (uint32_t)p_flag) == (uint32_t)p_flag;
}

void PKShaderMaterial::set_variation_flag(VariationFlags p_flag) {
	variation_flags = (VariationFlags)((uint32_t)variation_flags | (uint32_t)p_flag);
}

String PKShaderMaterial::_get_shader_path() const {
	// TODO: Hardcoded for now
	switch (renderer_type) {
		case Renderer_Ribbon:
			return "res://addons/popcornfx/assets/materials/popcornfx_ribbon.gdshader";
		case Renderer_Mesh:
			return "res://addons/popcornfx/assets/materials/popcornfx_mesh.gdshader";
		default:
			return "res://addons/popcornfx/assets/materials/popcornfx_billboard.gdshader";
	}
}
