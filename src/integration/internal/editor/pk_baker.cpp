//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#if TOOLS_ENABLED
#include "pk_baker.h"

#include "integration/internal/pk_project_settings.h"
#include "integration/pk_plugin.h"

#include <pk_base_object/include/hbo_object.h>
#include <pk_kernel/include/kr_resources.h>

#include <PK-AssetBakerLib/AssetBaker_Cookery.h>
#include <PK-AssetBakerLib/AssetBaker_Oven_HBO.h>
#include <PK-AssetBakerLib/AssetBaker_Oven_Texture.h>

#include <pk_kernel/include/kr_delegates.h>

using namespace godot;

PKBakeContext::~PKBakeContext() {
	if (bake_resource_manager != nullptr) {
		PKGD_ASSERT(bake_resource_mesh_handler != nullptr);
		PKGD_ASSERT(bake_resource_image_handler != nullptr);
		PKGD_ASSERT(bake_resource_rectangle_list_handler != nullptr);
		PKGD_ASSERT(bake_resource_fontmetrics_handler != nullptr);
		PKGD_ASSERT(bake_resource_vectorfield_handler != nullptr);

		bake_resource_manager->UnregisterHandler<CResourceMesh>(bake_resource_mesh_handler);
		bake_resource_manager->UnregisterHandler<CImage>(bake_resource_image_handler);
		bake_resource_manager->UnregisterHandler<CRectangleList>(bake_resource_rectangle_list_handler);
		bake_resource_manager->UnregisterHandler<CFontMetrics>(bake_resource_fontmetrics_handler);
		bake_resource_manager->UnregisterHandler<CVectorField>(bake_resource_vectorfield_handler);
	}
	PK_SAFE_DELETE(bake_resource_mesh_handler);
	PK_SAFE_DELETE(bake_resource_image_handler);
	PK_SAFE_DELETE(bake_resource_rectangle_list_handler);
	PK_SAFE_DELETE(bake_resource_fontmetrics_handler);
	PK_SAFE_DELETE(bake_resource_vectorfield_handler);
	PK_SAFE_DELETE(bake_context);
	PK_SAFE_DELETE(bake_fs_controller);
	PK_SAFE_DELETE(bake_resource_manager);
}

bool PKBakeContext::init() {
	PKGD_ASSERT(bake_fs_controller == nullptr);
	PKGD_ASSERT(bake_resource_manager == nullptr);
	PKGD_ASSERT(bake_context == nullptr);
	PKGD_ASSERT(bake_resource_mesh_handler == nullptr);
	PKGD_ASSERT(bake_resource_image_handler == nullptr);
	PKGD_ASSERT(bake_resource_rectangle_list_handler == nullptr);
	PKGD_ASSERT(bake_resource_fontmetrics_handler == nullptr);
	PKGD_ASSERT(bake_resource_vectorfield_handler == nullptr);

	// Keep this updated with all PopcornFX resource types
	bake_resource_mesh_handler = PK_NEW(CResourceHandlerMesh);
	bake_resource_image_handler = PK_NEW(CResourceHandlerImage);
	//bake_resource_image_handler = PK_NEW(CResourceHandlerImageGodot); // FIXME: We want to use the custom ResourceHandler but it causes a deadlock.
	bake_resource_rectangle_list_handler = PK_NEW(CResourceHandlerRectangleList);
	bake_resource_fontmetrics_handler = PK_NEW(CResourceHandlerFontMetrics);
	bake_resource_vectorfield_handler = PK_NEW(CResourceHandlerVectorField);
	if (!PKGD_VERIFY(bake_resource_mesh_handler != nullptr) ||
			!PKGD_VERIFY(bake_resource_image_handler != nullptr) ||
			!PKGD_VERIFY(bake_resource_rectangle_list_handler != nullptr) ||
			!PKGD_VERIFY(bake_resource_fontmetrics_handler != nullptr) ||
			!PKGD_VERIFY(bake_resource_vectorfield_handler != nullptr)) {
		return false;
	}

	bake_fs_controller = File::DefaultFileSystem();
	if (!PKGD_VERIFY(bake_fs_controller != nullptr)) {
		return false;
	}

	bake_resource_manager = PK_NEW(CResourceManager(bake_fs_controller));
	if (!PKGD_VERIFY(bake_resource_manager != nullptr)) {
		return false;
	}
	bake_resource_manager->RegisterHandler<CResourceMesh>(bake_resource_mesh_handler);
	bake_resource_manager->RegisterHandler<CImage>(bake_resource_image_handler);
	bake_resource_manager->RegisterHandler<CRectangleList>(bake_resource_rectangle_list_handler);
	bake_resource_manager->RegisterHandler<CFontMetrics>(bake_resource_fontmetrics_handler);
	bake_resource_manager->RegisterHandler<CVectorField>(bake_resource_vectorfield_handler);

	bake_context = PK_NEW(HBO::CContext(bake_resource_manager));
	if (!PKGD_VERIFY(bake_context != nullptr)) {
		return false;
	}
	return true;
}

bool PKBakeContext::remap_path(CString &p_path) {
#if 0
	if (p_path.EndsWith(".pkfx", CaseInsensitive))
		p_path = CFilePath::StripExtension(p_path) + ".pkb";
#endif

	return true;
}

PKBaker::~PKBaker() {
	PK_SAFE_DELETE(bake_context);
}

bool PKBaker::initialize_ifn() {
	if (initialized) {
		return true;
	}

	bake_context = PK_NEW(PKBakeContext());
	if (!PKGD_VERIFY(bake_context != nullptr && bake_context->init())) {
		CLog::Log(PK_ERROR, "Couldn't Initialize Cookery: Failed to create SBakeContext.");
		return false;
	}

	cookery.SetHBOContext(bake_context->bake_context); // init m_Cookery with correct context

	PKPlugin *plugin = PKPlugin::get_singleton();
	const CString root_dir = plugin->get_processed_project_settings()->get_source_pack_root_dir();
	bake_context->bake_fs_controller->MountPack(root_dir);

	if (!cookery.TurnOn()) {
		CLog::Log(PK_ERROR, "Couldn't Initialize Cookery: Failed to TurnOn().");
		return false;
	}

	COven_Particle *oven_particle = PK_NEW(COven_Particle);
	oven_particle->SetExternalPathRemapper(FastDelegate<bool(CString &)>(PKBakeContext::remap_path));

	const CGuid oven_id_particle = cookery.RegisterOven(oven_particle);
	const CGuid oven_id_texture = cookery.RegisterOven(PK_NEW(COven_Texture));

	if (!oven_id_particle.Valid() || !oven_id_texture.Valid()) {
		return false;
	}

	// map all known extensions to the appropriate oven:
#if 0
	cookery.MapOven("pkri", ovenIdStraightCopy);	// Editor Material
	cookery.MapOven("pkma", ovenIdStraightCopy);	// Editor Material

	cookery.MapOven("fbx", ovenIdMesh);				// FBX mesh
	cookery.MapOven("pkmm", ovenIdMesh);			// PopcornFX multi-mesh
#endif
	cookery.MapOven("dds", oven_id_texture); // dds image
	cookery.MapOven("png", oven_id_texture); // png image
	cookery.MapOven("jpg", oven_id_texture); // jpg image
	cookery.MapOven("jpeg", oven_id_texture); // jpg image
	cookery.MapOven("tga", oven_id_texture); // tga image
	cookery.MapOven("tif", oven_id_texture); // tiff image
	cookery.MapOven("tiff", oven_id_texture); // tiff image
	cookery.MapOven("pkm", oven_id_texture); // pkm image
	cookery.MapOven("pvr", oven_id_texture); // pvrtc image
	cookery.MapOven("hdr", oven_id_texture); // hdr image
#if 0
	cookery.MapOven("exr", oven_id_texture);			// exr image ------------- Collide with FBX
	cookery.MapOven("txt", ovenIdStraightCopy);		// misc
	cookery.MapOven("fga", ovenIdVectorField);		// FGA vector-field
	cookery.MapOven("pkfm", ovenIdStraightCopy);	// PopcornFX font
	cookery.MapOven("pkvf", ovenIdStraightCopy);	// PopcornFX vector-field
	cookery.MapOven("pkat", ovenIdTextureAtlas);	// PopcornFX atlas definition
	cookery.MapOven("pksc", ovenIdStraightCopy);	// PopcornFX simulation cache
	cookery.MapOven("pkbo", ovenIdHBO);				// PopcornFX base object
	cookery.MapOven("pkan", ovenIdHBO);				// PopcornFX Animation
	cookery.MapOven("pksa", ovenIdHBO);				// PopcornFX Skeletal Animation
	cookery.MapOven("mp3", ovenIdAudio);			// mp3 sound
	cookery.MapOven("wav", ovenIdAudio);			// wav sound
	cookery.MapOven("ogg", ovenIdAudio);			// ogg sound
#endif
	cookery.MapOven("pkfx", oven_id_particle);

	cookery.AddOvenFlags(COven::Flags_BakeMemoryVersion); // Do not use the 'Cache' directory of the source project
	cookery.AddOvenFlags(COven::Flags_ForcedOutputPath); //No directory structure
	cookery.AddOvenFlags(COven::Flags_ForceRebuild);

	// Setup the config file from scratch
	PBaseObjectFile config_file = cookery.m_BaseConfigFile; // Grab the base config file created by the m_Cookery
	if (!PKGD_VERIFY(config_file != nullptr)) // the base m_Cookery should always create a cfg object file
	{
		CLog::Log(PK_ERROR, "Couldn't Initialize Cookery: Failed to get configFile.");
		return false;
	}
	config_file->DeleteAllObjects(); // start from a blank state in case it succeeded in loading the global cfg file

	// Single config object for particles
	POvenBakeConfig_Particle bake_config = cookery.HBOContext()->NewObject<COvenBakeConfig_Particle>(config_file.Get());
	if (!PKGD_VERIFY(bake_config != nullptr)) {
		CLog::Log(PK_ERROR, "Couldn't Initialize Cookery: Failed to create bakeConfig.");
		return false;
	}

	bake_config->SetSourceConfig(Bake_NoSource);
	bake_config->SetCompile(true);
	bake_config->SetRemoveEditorNodes(true);

	const bool debug_baked_effects = PKPlugin::get_singleton()->get_processed_project_settings()->settings().debug_baked_effects;
	bake_config->SetBakeMode(debug_baked_effects ? COvenBakeConfig_HBO::Bake_SaveAsText : COvenBakeConfig_HBO::Bake_SaveAsBinary);

	// build versions
	_set_build_version(bake_config, "Editor"); // DEBUG

	// Nothing to remap
	const COvenBakeConfig_HBO::_TypeOfExtensionsRemap extensions_remap;

	bake_config->SetExtensionsRemap(extensions_remap);

	initialized = true;
	return true;
}

bool PKBaker::bake_effect(const CString &p_src_file_path, const CString &r_file_path, const CString &p_target_platform_name /*= "Editor"*/) {
	PK_SCOPEDLOCK(lock); //DEBUG

	if (!initialize_ifn()) {
		return false;
	}

	CLog::Log(PK_INFO, "Begin effect baking '%s' for '%s'...", p_src_file_path.Data(), p_target_platform_name.Data());

	const CString out_dir = CFilePath::StripFilename(r_file_path);
	const SBakeTarget default_target("Godot_Generic", out_dir);

	PKPlugin *plugin = PKPlugin::get_singleton();
	const CString root_dir = plugin->get_processed_project_settings()->get_source_pack_root_dir();

	if (cookery.m_DstPackPaths.Count() == 0) {
		cookery.m_DstPackPaths.PushBack(default_target);
	}
	CMessageStream baker_messages;

	const CString rel_src_file_path = CFilePath::Relativize(root_dir.Data(), p_src_file_path.Data());
	if (!cookery.BakeAsset(rel_src_file_path, cookery.m_BaseConfigFile, baker_messages)) {
		CLog::Log(PK_ERROR, "Failed baking effect '%s' for '%s'%s", rel_src_file_path.Data(), p_target_platform_name.Data(), !baker_messages.Messages().Empty() ? ":" : ".");
		_log_baker_messages(baker_messages, CMessageStream::Warning);
		return false;
	} else if (PKPlugin::get_singleton()->get_processed_project_settings()->settings().debug_baked_effects) {
		_log_baker_messages(baker_messages, CMessageStream::Info);
	}

	CLog::Log(PK_INFO, "Successfully baked effect '%s' for '%s'...", p_src_file_path.Data(), p_target_platform_name.Data());
	return true;
}

void PKBaker::_set_build_version(POvenBakeConfig_Particle &p_config, const CString &p_platform) {
	CString platform_tags;
	if (p_platform == "Editor") {
		platform_tags = "editor, desktop, windows, macos, linux";
	} else if (p_platform == "AllDesktop") {
		platform_tags = "desktop, windows, macos, linux";
	} else if (p_platform == "AllMobile") {
		platform_tags = "mobile, android, ios";
	}

	COvenBakeConfig_Particle::_TypeOfBuildVersions build_versions;
	PKGD_VERIFY(build_versions.PushBack(CString("Default: ") + platform_tags).Valid());

	p_config->SetBuildVersions(build_versions);
}

void PKBaker::_log_baker_messages(const CMessageStream &p_messages, CMessageStream::ELevel p_min_level) {
#if (PK_MESSAGE_STREAM_NO_MESSAGES == 0)
	TArray<CString> message_lines;
	for (const CMessageStream::SMessage &msg : p_messages.Messages()) {
		if (msg.m_Level < p_min_level) {
			continue;
		}
		msg.m_Message.Split('\n', message_lines);
		for (uint32_t j = 0; j < message_lines.Count(); j++) {
			switch (msg.m_Level) {
				case CMessageStream::Info:
					CLog::Log(PK_INFO, "\t- %s", message_lines[j].Data());
					break;
				case CMessageStream::Warning:
					CLog::Log(PK_WARN, "\t- %s", message_lines[j].Data());
					break;
				case CMessageStream::Error:
					CLog::Log(PK_ERROR, "\t- %s", message_lines[j].Data());
					break;
				default:
					PK_ASSERT_NOT_REACHED();
					break;
			}
		}
	}
#endif // (PK_MESSAGE_STREAM_NO_MESSAGES == 0)
}
#endif // TOOLS_ENABLED
