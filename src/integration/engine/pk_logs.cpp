//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include "integration/pk_sdk.h"

#include <pk_kernel/include/kr_log_listeners.h>

using namespace godot;

#define GODOT_PRINT(text) UtilityFunctions::print(Variant(text))

#ifndef PK_RETAIL

class CLogListenerGodot : public ILogListener {
public:
	virtual void Notify(CLog::ELogLevel p_level, CGuid p_log_class, const char *p_message) override {
		const CString s = CString::Format("[%s] %s", CLog::LogClassToString(p_log_class), p_message);
		switch (p_level) {
			case CLog::Level_Debug:
				GODOT_PRINT(s.Data());
				break;
			case CLog::Level_Info:
				GODOT_PRINT(s.Data());
				break;
			case CLog::Level_Warning:
			case CLog::Level_ErrorInternal: /* asserts */
				WARN_PRINT(s.Data());
				break;
			case CLog::Level_Error:
			case CLog::Level_ErrorCritical:
				ERR_PRINT(s.Data());
				break;
			case CLog::Level_None: /* used mainly to disable all logging when calling 'SetGlobalLogLevel()' */
				break;
			default:
				break;
		};
	}
};
#endif

void add_default_global_listeners_override(void *p_user_handle) {
#ifndef PK_RETAIL
	CLog::AddGlobalListener(PK_NEW(CLogListenerGodot));
#else
	(void)p_user_handle;
#endif
}
