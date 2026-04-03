//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_project_settings.h"

#include "godot_cpp/variant/variant.hpp"

#include "integration/engine/pk_file_system_controller.h"
#include "integration/pk_plugin.h"

#include "editor/pk_baker.h"

#include <pk_kernel/include/kr_refcounted_buffer.h>
#include <pk_kernel/include/kr_streams_memory.h>
#include <pk_particles/include/ps_project_settings.h>

bool PKProjectSettings::load_ifn() {
	// Do not hot reload pack
	if (source_pack_found) {
		return true;
	}

	PKFileSystemController *fs = PKFileSystemController::get_default();

#if TOOLS_ENABLED
	if (engine_settings.source_pack == "") {
		WARN_PRINT(vformat("PopcornFX: No Source Pack set. Please set this setting under PopcornFX/Editor."));
		return false;
	}

	if (!engine_settings.source_pack.Empty()) {
		const CString file_buffer = fs->BufferizeToString(engine_settings.source_pack, true);
		if (file_buffer == nullptr) {
			CLog::Log(PK_ERROR, "Failed to read project settings file at \"%s\"", engine_settings.source_pack.Data());
			return false;
		}

		CConstMemoryStream stream(file_buffer.Data(), file_buffer.Length());
		HBO::CContext *local_bo_context = PK_NEW(PopcornFX::HBO::CContext());

		if (PKGD_VERIFY(local_bo_context != nullptr)) {
			PProjectSettings project_settings = CProjectSettings::LoadFromStream(stream, local_bo_context);
			if (project_settings == nullptr) {
				CLog::Log(PK_ERROR, "Failed to load project settings file at \"%s\"", engine_settings.source_pack.Data());
				return false;
			}
			project_settings->ApplyGlobalSettings();

			const PProjectSettingsGeneral general = project_settings->General();

			const CString path = CFilePath::StripFilename(engine_settings.source_pack);
			source_pack_project_file = engine_settings.source_pack;
			engine_settings.source_pack_root = path / general->RootDir();
			source_pack_library_dir = path / general->LibraryDir();
			source_pack_thumbnails_dir = path / general->ThumbnailsDir();
			source_pack_cache_dir = path / general->EditorCacheDir();

			local_bo_context->UnloadAllFiles();
			PK_DELETE(local_bo_context);
		}
	}
#endif // TOOLS_ENABLED

	if (engine_settings.source_pack_root.Empty()) {
		ERR_FAIL_V_MSG(false, "PopcornFX pack root setting is empty. Was this project ever loaded in the Godot editor?");
	}

	fs->set_source_pack_root_dir(to_gd(engine_settings.source_pack_root));
	const PFilePack pack = fs->MountPack(engine_settings.source_pack_root);

	if (pack != nullptr) {
		source_pack_found = true;
	} else {
		ERR_FAIL_V_MSG(false, vformat("Could not mount the PopcornFX pack: %s", engine_settings.source_pack_root.Data()));
	}

	return true;
}
