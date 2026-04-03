//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "integration/pk_sdk.h"

class PKProjectSettings {
public:
	struct EngineSettings {
#if TOOLS_ENABLED
		CString source_pack;
		bool debug_baked_effects = false;
#endif
		CString source_pack_root;
	};

	PKProjectSettings() = default;
	~PKProjectSettings() = default;

	bool load_ifn();

	void set_settings(const EngineSettings &p_settings) { engine_settings = p_settings; }
	const EngineSettings &settings() const { return engine_settings; }
	const CString &get_source_pack_root_dir() const { return engine_settings.source_pack_root; }

private:
	EngineSettings engine_settings;

	CString source_pack_project_file;
	CString source_pack_library_dir;
	CString source_pack_cache_dir;
	CString source_pack_thumbnails_dir;
	bool source_pack_found = false;
};
