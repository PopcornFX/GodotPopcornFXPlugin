//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#include "godot_cpp/core/error_macros.hpp"

#include "pk_sdk.h"

namespace godot {
inline bool err_assert(const char *p_function, const char *p_file, int p_line, bool p_cond, const char *p_error) {
	if (unlikely(!p_cond)) {
		_err_print_error(p_function, p_file, p_line, p_error);
	}
	return p_cond;
}
inline bool err_assert_msg(const char *p_function, const char *p_file, int p_line, bool p_cond, const char *p_message, ...) {
	if (unlikely(!p_cond)) {
		va_list arg_list;
		char formatted_msg[256];
		va_start(arg_list, p_message); // message is the format
		SNativeStringUtils::VSPrintf(formatted_msg, p_message, arg_list);
		va_end(arg_list);

		_err_print_error(p_function, p_file, p_line, formatted_msg);
	}
	return p_cond;
}
} //namespace godot

#define GD_ASSERT(m_cond) \
	::godot::err_assert(FUNCTION_STR, "[PopcornFX] " __FILE__, __LINE__, m_cond, "Condition \"" _STR(m_cond) "\" is false.")

#define GD_ASSERT_MSG(m_cond, m_msg, ...) \
	::godot::err_assert_msg(FUNCTION_STR, "[PopcornFX] " __FILE__, __LINE__, m_cond, m_msg, ##__VA_ARGS__)

#if defined(DEBUG_ENABLED)

#define PKGD_ASSERT(m_cond) \
	GD_ASSERT(m_cond);      \
	PK_ASSERT(m_cond);

#define PKGD_ASSERT_MSG(m_cond, m_msg, ...)      \
	GD_ASSERT_MSG(m_cond, m_msg, ##__VA_ARGS__); \
	PK_ASSERT_MESSAGE(m_cond, m_msg, ##__VA_ARGS__);

#define PKGD_VERIFY(m_cond) \
	(GD_ASSERT(m_cond) || PK_VERIFY(m_cond))

#define PKGD_VERIFY_MSG(m_cond, m_msg, ...) \
	(GD_ASSERT_MSG(m_cond, m_msg, ##__VA_ARGS__) || PK_VERIFY_MESSAGE(m_cond, m_msg, ##__VA_ARGS__))

#else // defined(DEBUG_ENABLED)

#define PKGD_ASSERT(m_cond) \
	PK_ASSERT(m_cond);

#define PKGD_ASSERT_MSG(m_cond, m_msg, ...) \
	PK_ASSERT_MESSAGE(m_cond, m_msg, ##__VA_ARGS__);

#define PKGD_VERIFY(m_cond) \
	PK_VERIFY(m_cond)

#define PKGD_VERIFY_MSG(m_cond, m_msg, ...) \
	PK_VERIFY_MESSAGE(m_cond, m_msg, ##__VA_ARGS__)

#endif // defined(DEBUG_ENABLED)
