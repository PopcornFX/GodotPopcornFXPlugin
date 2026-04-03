//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

bool popcornfx_startup();
void popcornfx_shutdown();

#define STARTUP_PLUGIN(__path, __sym)                                                            \
	do {                                                                                         \
		const char *pluginPath = __path PK_PLUGIN_POSTFIX_BUILD PK_PLUGIN_POSTFIX_EXT;           \
		IPluginModule *plugin = PK_GLUE(StartupPlugin_, __sym());                                \
		success &= (plugin != null && CPluginManager::PluginRegister(plugin, true, pluginPath)); \
	} while (0)

#define SHUTDOWN_PLUGIN(__sym)                                     \
	do {                                                           \
		IPluginModule *plugin = PK_GLUE(GetPlugin_, __sym());      \
		(plugin != null && CPluginManager::PluginRelease(plugin)); \
		PK_GLUE(ShutdownPlugin_, __sym());                         \
	} while (0)
