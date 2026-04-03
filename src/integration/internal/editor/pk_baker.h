//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#if TOOLS_ENABLED
#include "integration/pk_sdk.h"

#include <PK-AssetBakerLib/AssetBaker_Cookery.h>
#include <pk_geometrics/include/ge_mesh_resource_handler.h>
#include <pk_geometrics/include/ge_rectangle_list.h>
#include <pk_imaging/include/im_resource.h>
#include <pk_kernel/include/kr_messages.h>
#include <pk_particles/include/ps_font_metrics_resource.h>
#include <pk_particles/include/ps_vectorfield_resource.h>

__PK_API_BEGIN
PK_FORWARD_DECLARE(OvenBakeConfig_Particle);
PK_FORWARD_DECLARE(IFileSystem);
class CResourceManager;
namespace HBO {
class CContext;
}
__PK_API_END

struct PKBakeContext {
	IFileSystem *bake_fs_controller = nullptr;
	CResourceManager *bake_resource_manager = nullptr;
	HBO::CContext *bake_context = nullptr;
	CResourceHandlerImage *bake_resource_image_handler = nullptr;
	CResourceHandlerMesh *bake_resource_mesh_handler = nullptr;
	CResourceHandlerRectangleList *bake_resource_rectangle_list_handler = nullptr;
	CResourceHandlerVectorField *bake_resource_vectorfield_handler = nullptr;
	CResourceHandlerFontMetrics *bake_resource_fontmetrics_handler = nullptr;

	PKBakeContext() = default;
	~PKBakeContext();
	bool init();

	static bool remap_path(CString &p_path);
};

class PKBaker {
public:
	PKBaker() = default;
	~PKBaker();

	bool initialize_ifn();
	bool bake_effect(const CString &p_src_file_path, const CString &r_file_path, const CString &p_target_platform_name = "Editor");

private:
	void _set_build_version(POvenBakeConfig_Particle &p_config, const CString &p_platform);
	void _log_baker_messages(const CMessageStream &p_messages, CMessageStream::ELevel p_min_level);

	Threads::CCriticalSection lock; //DEBUG
	CCookery cookery;
	PKBakeContext *bake_context = nullptr;

	bool initialized = false;
};
#endif
