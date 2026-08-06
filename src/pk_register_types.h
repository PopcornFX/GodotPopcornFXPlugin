//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

#define DEFINE_BASIC_PROPERTY(type, name) \
private:                                  \
	type name;                            \
                                          \
public:                                   \
	void set_##name(type name) {          \
		this->name = name;                \
	}                                     \
	type get_##name() const {             \
		return name;                      \
	}
#define BIND_BASIC_PROPERTY(bind_type, type, name, ...) \
	BIND_BASIC_PROPERTY_BASE(bind_type, type, name, _, __VA_ARGS__)

#define BIND_BASIC_PUBLIC_PROPERTY(bind_type, type, name, ...) \
	BIND_BASIC_PROPERTY_BASE(bind_type, type, name, , __VA_ARGS__)

#define BIND_BASIC_PROPERTY_BASE(bind_type, type, name, prefix, ...)                     \
	ClassDB::bind_method(D_METHOD(#prefix "set_" #name, #name), &bind_type::set_##name); \
	ClassDB::bind_method(D_METHOD(#prefix "get_" #name), &bind_type::get_##name);        \
	ADD_PROPERTY(PropertyInfo(Variant::type, #name, __VA_ARGS__), #prefix "set_" #name, #prefix "get_" #name)
