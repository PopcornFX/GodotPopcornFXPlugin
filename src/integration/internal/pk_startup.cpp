//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "pk_startup.h"

#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/variant.hpp"

#include "integration/engine/pk_file_system_controller.h"
#include "integration/internal/pk_resource_handler_image.h"
#include "integration/pk_error_handling.h"
#include "integration/pk_sdk.h"

// init
#include <pk_base_object/include/hb_init.h>
#include <pk_compiler/include/cp_init.h>
#include <pk_engine_utils/include/eu_init.h>
#include <pk_geometrics/include/ge_coordinate_frame.h>
#include <pk_geometrics/include/ge_init.h>
#include <pk_imaging/include/im_init.h>
#include <pk_kernel/include/kr_init.h>
#include <pk_kernel/include/kr_static_config_flags.h>
#include <pk_particles/include/ps_init.h>
#include <pk_particles_toolbox/include/pt_init.h>
#include <pk_render_helpers/include/rh_init.h>
#include <pk_version_base.h>

#include <pk_kernel/include/kr_mem_stats.h>
#include <pk_kernel/include/kr_plugins.h>

#include <pk_toolkit/include/pk_toolkit_version.h>

#ifdef TOOLS_ENABLED
#include <PK-AssetBakerLib/AssetBaker_Startup.h>
#endif // TOOLS_ENABLED

#if (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)
#include "godot_cpp/classes/os.hpp"
#endif // (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)

PK_LOG_MODULE_DECLARE();

#ifdef PK_PLUGINS_STATIC

PK_PLUGIN_DECLARE(CCompilerBackendCPU_VM);
PK_PLUGIN_DECLARE(CImagePKIMCodec);
PK_PLUGIN_DECLARE(CImageDDSCodec);
PK_PLUGIN_DECLARE(CImagePNGCodec);
PK_PLUGIN_DECLARE(CImageTGACodec);
PK_PLUGIN_DECLARE(CImageJPEGCodec);
//PK_PLUGIN_DECLARE(CImageTIFFCodec); // Link issue, temp disable
PK_PLUGIN_DECLARE(CImagePKMCodec);
PK_PLUGIN_DECLARE(CImagePVRCodec);
PK_PLUGIN_DECLARE(CImageHDRCodec);
PK_PLUGIN_DECLARE(CImageEXRCodec);

#if (PK_COMPILE_GPU_D3D == 1)
PK_PLUGIN_DECLARE(CCompilerBackendGPU_D3D);
#endif

#ifdef PK_DEBUG
#define PK_PLUGIN_POSTFIX_BUILD "_D"
#else
#define PK_PLUGIN_POSTFIX_BUILD ""
#endif
#if defined(PK_WINDOWS)
#define PK_PLUGIN_POSTFIX_EXT ".dll"
#else
#define PK_PLUGIN_POSTFIX_EXT ""
#endif
#endif

using namespace godot;

void add_default_global_listeners_override(void *p_user_handle);

namespace {

void godot_alloc_failed(bool *r_retry_alloc, ureg p_size, uint32_t p_alignment, PopcornFX::Mem::EAllocType p_type) {
	ERR_PRINT(vformat("PopcornFX went out of memory for %llu bytes !", static_cast<uint64_t>(p_size)));
	ERR_PRINT(vformat("PopcornFX memory stats in bytes: "));
	ERR_PRINT(vformat("    footprint:       %d (overhead: %d)", CMemStats::RealFootprint(), CMemStats::Overhead()));
	//OnOutOfMemory(size, alignment);
}

void *godot_alloc(size_t p_size, PopcornFX::Mem::EAllocType p_type) {
	(void)p_type;
	return memalloc(p_size); // popcornfx takes care of alignment already
}

void *godot_realloc(void *p_ptr, size_t p_size, PopcornFX::Mem::EAllocType p_type) {
	(void)p_type;
	return memrealloc(p_ptr, p_size);
}

void godot_free(void *p_ptr, PopcornFX::Mem::EAllocType p_type) {
	(void)p_type;
	return memfree(p_ptr);
}

IFileSystem *popcornfx_create_new_file_system() {
	return PK_NEW(PKFileSystemController);
}

#if (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)

void pretty_format_assert_unsafe(char *p_buffer, u32 p_buffer_len, PK_ASSERT_CATCHER_PARAMETERS) {
	PK_ASSERT_CATCHER_KILLARGS;
	using namespace PopcornFX;

	const CThreadID thread_id = CCurrentThread::ThreadID();

#if (PK_CALL_SCOPE_ENABLED != 0)
	const PopcornFX::CString call_ctx_str = CCallContext::ReadStackToString();
	const char *call_ctx_str_ptr = call_ctx_str.Data();
#else
	const char *callCtxStrPtr = null;
#endif //(PK_CALL_SCOPE_ENABLED != 0)

	// pointer compare ok (compiler probably removed duplicated static strings, else we dont care)
	const bool pexp = (expanded != failed);
	const bool pmsg = (message != failed && message != expanded);
	const bool cctx = call_ctx_str_ptr != null;
	SNativeStringUtils::SPrintf(
			p_buffer, p_buffer_len,
			"!! PopcornFX Assertion failed !!"
			"\nFile       : %s(%d)"
			"\nFunction   : %s(...)"
			"%s%s" // Message
			"\nCondition  : %s"
			"%s%s" // Expanded
			"%s%s" // Call context
			"\nThreadID   : %d"
			"\n",
			file, line, function, (pmsg ? "\nMessage    : " : ""), (pmsg ? message : ""), failed, (pexp ? "\nExpanded   : " : ""), (pexp ? expanded : ""), (cctx ? "\nCallContext: " : ""), (cctx ? call_ctx_str_ptr : ""), u32(thread_id));
}

PopcornFX::Assert::EResult popcornfx_assert(PK_ASSERT_CATCHER_PARAMETERS) {
	PK_ASSERT_CATCHER_KILLARGS;

	char buffer[2048];
	pretty_format_assert_unsafe(buffer, sizeof(buffer), PK_ASSERT_CATCHER_ARGUMENTS);

	ERR_PRINT(godot::String(buffer));
	OS::get_singleton()->alert(String(buffer), "PopcornFX Assertion Failed");
	PK_BREAKPOINT();

	return PopcornFX::Assert::Result_Ignore;
}

#endif // (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)

bool startup_plugins() {
	using namespace PopcornFX;

	bool success = true;

#ifdef PK_PLUGINS_STATIC
	STARTUP_PLUGIN("Plugins/CBCPU_VM", CCompilerBackendCPU_VM);

	// Image loaders
	STARTUP_PLUGIN("Plugins/image_codec_pkim", CImagePKIMCodec);
	STARTUP_PLUGIN("Plugins/image_codec_dds", CImageDDSCodec);
	STARTUP_PLUGIN("Plugins/image_codec_png", CImagePNGCodec);
	STARTUP_PLUGIN("Plugins/image_codec_tga", CImageTGACodec);
	STARTUP_PLUGIN("Plugins/image_codec_jpeg", CImageJPEGCodec);
	//STARTUP_PLUGIN("Plugins/image_codec_tiff", CImageTIFFCodec);
	STARTUP_PLUGIN("Plugins/image_codec_pvr", CImagePVRCodec);
	STARTUP_PLUGIN("Plugins/image_codec_pkm", CImagePKMCodec);
	STARTUP_PLUGIN("Plugins/image_codec_hdr", CImageHDRCodec);
#if defined(PK_WINDOWS) || defined(PK_MACOSX)
	STARTUP_PLUGIN("Plugins/image_codec_exr", CImageEXRCodec);
#endif

	return success;
#else
	TODO
#endif
}

void shutdown_plugins() {
	using namespace PopcornFX;

#ifdef PK_PLUGINS_STATIC
	SHUTDOWN_PLUGIN(CCompilerBackendCPU_VM);

	// Image loaders
	SHUTDOWN_PLUGIN(CImagePKIMCodec);
	SHUTDOWN_PLUGIN(CImageDDSCodec);
	SHUTDOWN_PLUGIN(CImagePNGCodec);
	SHUTDOWN_PLUGIN(CImageTGACodec);
	SHUTDOWN_PLUGIN(CImageJPEGCodec);
	//SHUTDOWN_PLUGIN(CImageTIFFCodec);
	SHUTDOWN_PLUGIN(CImagePVRCodec);
	SHUTDOWN_PLUGIN(CImagePKMCodec);
	SHUTDOWN_PLUGIN(CImageHDRCodec);
#if defined(PK_WINDOWS) || defined(PK_MACOSX)
	SHUTDOWN_PLUGIN(CImageEXRCodec);
#endif
#else
	TODO
#endif
}

} // namespace

bool popcornfx_startup() {
	using namespace PopcornFX;

	SDllVersion engine_version;
	bool success = false;

	if (engine_version.Major != PK_VERSION_MAJOR || engine_version.Minor != PK_VERSION_MINOR) {
		ERR_PRINT(vformat("PopcornFX Runtime version missmatch: PopcornFX Runtime is v%d.%d, but Plugin has been build with v%d.%d headers !", engine_version.Major, engine_version.Minor, PK_VERSION_MAJOR, PK_VERSION_MINOR));
		return false;
	}

	CPKKernel::Config kernel_configuration;

	kernel_configuration.m_AddDefaultLogListeners = &add_default_global_listeners_override;
	kernel_configuration.m_NewFileSystem = &popcornfx_create_new_file_system;

	//	kernelConfiguration.m_CreateThreadPool = &_CreateThreadPool_Godot_Auto;

	kernel_configuration.m_DefaultAllocator_Alloc = &godot_alloc;
	kernel_configuration.m_DefaultAllocator_Realloc = &godot_realloc;
	kernel_configuration.m_DefaultAllocator_Free = &godot_free;
	kernel_configuration.m_DefaultAllocator_OutOfMemory = &godot_alloc_failed;

#if !defined(PK_WINDOWS) && ((PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0))
	kernel_configuration.m_AssertCatcher = &popcornfx_assert;
#endif // (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)

	success = true;
	success &= CPKKernel::Startup(engine_version, kernel_configuration);
	success &= CPKBaseObject::Startup(engine_version, CPKBaseObject::Config());
	success &= CPKEngineUtils::Startup(engine_version, CPKEngineUtils::Config());
	success &= CPKCompiler::Startup(engine_version, CPKCompiler::Config());
	success &= CPKImaging::Startup(engine_version, CPKImaging::Config());
	success &= CPKGeometrics::Startup(engine_version, CPKGeometrics::Config());
	success &= CPKParticles::Startup(engine_version, CPKParticles::Config());
	success &= ParticleToolbox::Startup();
	success &= CPKRenderHelpers::Startup(engine_version, CPKRenderHelpers::Config());
	success &= PKGD_VERIFY(Kernel::CheckStaticConfigFlags(Kernel::g_BaseStaticConfig, SKernelConfigFlags()));
	if (!success) {
		ERR_PRINT("PopcornFX runtime failed to startup");
		popcornfx_shutdown();
		return false;
	}

	if (!startup_plugins()) {
		ERR_PRINT("PopcornFX runtime plugins failed to startup");
		popcornfx_shutdown();
		return false;
	}

	PK_LOG_MODULE_INIT_START;
	PK_LOG_MODULE_INIT_END;

	PKResourceHandlerImage::startup();

#ifdef TOOLS_ENABLED
	AssetBaker::Startup();
#endif // TOOLS_ENABLED

	CCoordinateFrame::SetGlobalFrame(ECoordinateFrame::Frame_RightHand_Y_Up);

	return success;
}

void popcornfx_shutdown() {
	using namespace PopcornFX;

	PK_LOG_MODULE_RELEASE_START;
	PK_LOG_MODULE_RELEASE_END;

	// Unload all PopcornFX files
	HBO::g_Context->UnloadAllFiles();

	PKResourceHandlerImage::shutdown();

#ifdef TOOLS_ENABLED
	AssetBaker::Shutdown();
#endif // TOOLS_ENABLED

	shutdown_plugins();

	CPKRenderHelpers::Shutdown();
	ParticleToolbox::Shutdown();
	CPKParticles::Shutdown();
	CPKGeometrics::Shutdown();
	CPKImaging::Shutdown();
	CPKCompiler::Shutdown();
	CPKEngineUtils::Shutdown();
	CPKBaseObject::Shutdown();
	CPKKernel::Shutdown();
}
